//SSOO-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Constantes:
#define MAX_OPERACIONES 200
#define MAX_DIGITOS_CANTIDAD_TRANSACCION 10

// Variables globales:
// Lista que contiene las operaciones en formato string
char **list_client_ops;
int client_numop = 0;
int bank_numop = 0;
int global_balance = 0;
int *saldo_cuenta;
pthread_mutex_t mutex;
queue *cola;

/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */

// Estructuras:
typedef struct crear_elem{
    int read_arg1;
    int read_arg2;
    int read_arg3;
    int max_cuentas;
    char *cuenta1_char;
    char *cuenta2_char;
    char cantidad_char[MAX_DIGITOS_CANTIDAD_TRANSACCION]; 
    int cuenta1; 
    int cuenta2;
    int cantidad;
    int i;
    int longitud;
    int n;
    int cambio;
}crear_elem_t;

typedef struct insertar_elem{
    char *operacion_str;
    operacion_t operacion;
    crear_elem_t variables;
    int digitos_max_cuentas;
}insertar_elem_t;

// Prototipos:
operacion_t crear_elemento_operacion(char *operacion_str, operacion_t op,   
                                    int read_arg1, int read_arg2, int read_arg3,
                                    int max_cuentas, char *cuenta1_char, char *cuenta2_char, 
                                    char *cantidad_char, int cuenta1, int cuenta2, 
                                    int cantidad, int i, int longitud, int n, int cambio);
void insertar_elemento_en_cola(insertar_elem_t *op);


 
int main (int argc, const char * argv[] ) {
    //./bank <nombre fichero> <num cajeros> <num trabajadores> <max cuentas>
    // <tam buff>

    // Comprobamos que el número de argumentos es correcto
    if (argc != 6){
        printf("Error: Número de argumentos incorrecto\n");
        return -1;
    }

    // Abrimos el fichero de entrada (operaciones)
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0){
        perror("Error: No se ha podido abrir el fichero\n");
        return -1;
    }
    
    // El segundo argumento nos especifica el número de cajeros (hilos productores)
    int num_cajeros;
    if ((num_cajeros = atoi(argv[2])) <= 0){
        printf("Error: El número de cajeros debe ser mayor que 0\n");
        return -1;
    }

    // El tercer argumento representa el número de trabajadores del banco (hilos consumidores)
    int num_empleados;
    if ((num_empleados = atoi(argv[3])) <= 0){
        printf("Error: El número de empleados debe ser mayor que 0\n");
        return -1;
    }

    // El cuarto argumento representa el número máximo de cuentas que puede haber en el banco
    int max_cuentas;
    if ((max_cuentas = atoi(argv[4])) <= 0){
        printf("Error: El número máximo de cuentas debe ser mayor que 0\n");
        return -1;
    }
    int digitos_max_cuentas = strlen(argv[4]);
    
    // El quinto argumento representa el tamaño de la cola circular
    // sobre el que se irán almacenando las operaciones
    int tam_buff;
    if ((tam_buff = atoi(argv[5])) <= 0){
        printf("Error: El tamaño del buffer debe ser mayor que 0\n");
        return -1;
    }
    cola = queue_init(tam_buff);

    // Leemos el fichero de entrada
    int bytes;
    // Diferenciador entre fichero vacío y fichero leído
    int vacio = 1;
    char buf[1];
    int contador = 0;
    char num_operaciones_char[3];
    int num_operaciones;
    int primer_char = 1;
    while ((bytes = read(fd, buf, 1)) > 0){ 
        vacio = 0;
        // Si estamos al principio del fichero, leemos primero 
        // el número de operaciones que se realizarán 
        if (contador == 0){
            if(buf[0] == '\n'){
                num_operaciones = atoi(num_operaciones_char);
                
                if (num_operaciones > MAX_OPERACIONES){
                    printf("Error: No se pueden superar las 200 operaciones máximas\n");
                    close(fd);
                    exit(-1);
                }
                // Creamos el array en el que se almacenará las operaciones
                // (el tamaño del array es el número de operaciones por el tamaño máximo que puede 
                // tener un string de operación = TRASPASAR+''+digitos_max_cuentas+''+digitos_max_cuentas+
                // ''+int_de_10_digitos)
                int tamaño_maximo_operacion_str = 9+1+(digitos_max_cuentas*2)+2+10;
                list_client_ops = (char**)malloc(sizeof(char*)*num_operaciones);
                for(int i=0; i<num_operaciones; i++){
                    list_client_ops[i] = (char*)malloc(tamaño_maximo_operacion_str*sizeof(char));
                }
                contador++;
            }
            else{
                strncat(num_operaciones_char, buf, 1);
            }
        }
        else{
            if(strncmp(buf, "\n", 1) != 0){
                // Añadimos la línea (una operación) a la lista
                if (primer_char == 1){
                    primer_char = 0;
                    strncpy(list_client_ops[contador-1], buf, 1); 
                }
                else{
                    strncat(list_client_ops[contador-1], buf, 1); 
                }
            }
            else{
                contador++;
            }
        }
    }

    if (bytes < 0){
        perror("Error: No se ha podido leer el fichero de entrada\n");
        close(fd);
        exit(-1);
    }
    
    if (vacio == 1){
        printf("Error: Fichero de entrada vacío\n");
        close(fd);
        exit(-1);
    }

    // En ningún caso puede haber menos o más operaciones que las indicadas
    // en el fichero
    if ((contador-1) != num_operaciones){
        printf("Error: En el fichero de entrada no hay el número de operaciones indicado\n");
        close(fd);
        exit(-1);
    }

    // Establecemos el mutex
    pthread_mutex_init(&mutex, NULL);
    
    // Creamos el cajero (hilo productor) y le pasamos una operación,
    // deben leer la operación indicada en client_numop
    pthread_t th[num_cajeros];
    operacion_t op[num_operaciones];
    insertar_elem_t ie[num_operaciones];
    while (client_numop <= num_operaciones){
        int i;
        for(i=0; i<num_cajeros; i++){
            printf("Creando hilo %d\n", i);        
            // Establecemos el número de operación correspondiente
            op[client_numop].num_operacion = client_numop+1;
            ie[client_numop].operacion_str = list_client_ops[client_numop];
            ie[client_numop].operacion = op[client_numop];
            ie[client_numop].digitos_max_cuentas = digitos_max_cuentas;
            ie[client_numop].variables.read_arg1 = 0;
            ie[client_numop].variables.read_arg2 = 0;
            ie[client_numop].variables.read_arg3 = 0;
            ie[client_numop].variables.max_cuentas = max_cuentas;
            ie[client_numop].variables.cuenta1_char = (char*)malloc(digitos_max_cuentas);
            strcpy(ie[client_numop].variables.cuenta1_char, " ");
            ie[client_numop].variables.cuenta2_char = (char*)malloc(digitos_max_cuentas);
            strcpy(ie[client_numop].variables.cuenta2_char, " ");
            strcpy(ie[client_numop].variables.cantidad_char, " ");
            ie[client_numop].variables.cuenta1 = -1; 
            ie[client_numop].variables.cuenta2 = -1;
            ie[client_numop].variables.cantidad = -1;
            ie[client_numop].variables.i = 0;
            ie[client_numop].variables.longitud = strlen(list_client_ops[client_numop]);
            ie[client_numop].variables.n = 0;
            ie[client_numop].variables.cambio = 0;
            
            pthread_create(&th[i], NULL, (void*)insertar_elemento_en_cola, &ie[client_numop]);
            client_numop++;
        }      
        int j;
        for(j=0; j<num_cajeros; j++){
            printf("Join hilo %d\n", j);
            pthread_join(th[j], NULL);
        }
    }
    // Tras el procesamiento de las operaciones liberamos la memoria 
    // reservada con malloc
    free(list_client_ops);

    return 0;
}

void insertar_elemento_en_cola(insertar_elem_t *param){
    //printf("entro\n");
    //printf("hilo: %ld\n", pthread_self());
    sleep(2);
    crear_elemento_operacion(param->operacion_str, param->operacion, param->variables.read_arg1,
                                param->variables.read_arg2, param->variables.read_arg3,
                                param->variables.max_cuentas, param->variables.cuenta1_char, 
                                param->variables.cuenta2_char, param->variables.cantidad_char,
                                param->variables.cuenta1, param->variables.cuenta2, param->variables.cantidad,
                                param->variables.i, param->variables.longitud, param->variables.n,
                                param->variables.cambio);
       
    // Lo escribimos en la cola, sin que el resto de hilos puedan escribir y en orden
    
    pthread_exit(NULL);
}

operacion_t crear_elemento_operacion(char *operacion_str, operacion_t op,   
                                    int read_arg1, int read_arg2, int read_arg3,
                                    int max_cuentas, char *cuenta1_char, char *cuenta2_char, 
                                    char *cantidad_char, int cuenta1, int cuenta2, 
                                    int cantidad, int i, int longitud, int n, int cambio){                       

    // Identificamos la operación
    if (strncmp(operacion_str, "CREAR ", 6) == 0){
        // Añadimos el tipo de operación
        strcpy(op.operacion, "CREAR");
        // Actualizamos el puntero de lectura del string
        i = 6;
    }
    else if (strncmp(operacion_str, "INGRESAR ", 9) == 0){
        strcpy(op.operacion, "INGRESAR");
        i = 9;
    }
    else if (strncmp(operacion_str, "TRASPASAR ", 10) == 0){
        strcpy(op.operacion, "TRASPASAR");
        i = 10;
    }
    else if (strncmp(operacion_str, "RETIRAR ", 8) == 0){
        strcpy(op.operacion, "RETIRAR");
        i = 8;
    }
    else if (strncmp(operacion_str, "SALDO ", 6) == 0){
        strcpy(op.operacion, "SALDO");
        i = 6;
    }
    else{
        printf("Error: Operación incorrecta\n");
        pthread_exit(NULL);
    }

    // Buscamos el número de cuenta y cantidad 
    while (i<longitud){
        if ((read_arg1 == 1 && cambio == 1) || 
            (read_arg2 == 1 && cambio == 1)){
            // Reiniciamos la posición del char dentro del argumento
            n = 0;
        }
        if ((operacion_str[i] != ' ') && (read_arg1 == 0)){
            cuenta1_char[n] = operacion_str[i];

            if ((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                read_arg1 = 1;
                cuenta1 = atoi(cuenta1_char);
                if (cuenta1 > max_cuentas){
                    printf("Error: Número máximo de cuentas excedido\n");
                    pthread_exit(NULL);
                }
                op.num_cuenta1 = cuenta1;
                cambio = 1;
            }
        }
        else if((operacion_str[i] != ' ') && (read_arg2 == 0)){
            cambio = 0;
            if (strncmp(op.operacion, "TRASPASAR", 9) != 0){
                if (n < MAX_DIGITOS_CANTIDAD_TRANSACCION){
                    cantidad_char[n] = operacion_str[i];
                }
                else{
                    printf("Error: No se admite esa cantidad, debe tener 10 dígitos como máximo\n");
                    pthread_exit(NULL);
                }

                if ((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                    read_arg2 = 1;
                    cantidad = atoi(cantidad_char); 
                    op.cantidad = cantidad;
                    cambio = 1;
                }
            }
            else if(strncmp(op.operacion, "CREAR", 5) == 0 || 
                    strncmp(op.operacion, "SALDO", 5) == 0 ){

                printf("Error: Número máximo de argumentos excedido\n");
                pthread_exit(NULL);
            }
            else{
                // Si es la operación TRASPASAR buscamos cuenta2
                cuenta2_char[n] = operacion_str[i];
                
                if ((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                    read_arg2 = 1;
                    cuenta2 = atoi(cuenta2_char);
                    if (cuenta2 > max_cuentas){
                        printf("Error: Número máximo de cuentas excedido\n");
                        pthread_exit(NULL);
                    }
                    op.num_cuenta2 = cuenta2;
                    cambio = 1;
                }
            }
        }
        // Si la operación es TRASPASAR buscamos la cantidad, si otra operación entra en el if
        // es que tiene una operación de más
        else if((operacion_str[i] != ' ') && (read_arg3 == 0)){
            cambio = 0;
            if (strncmp(op.operacion, "TRASPASAR", 9) == 0){
                if (n <= MAX_DIGITOS_CANTIDAD_TRANSACCION){
                    cantidad_char[n] = operacion_str[i];
                }
                else{
                    printf("Error: No se admite esa cantidad, debe tener 10 dígitos como máximo");
                    pthread_exit(NULL);
                }     

                if((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                    read_arg3 = 1;
                    cantidad = atoi(cantidad_char);
                    op.cantidad = cantidad;
                    cambio = 1;
                }
            }
            else{
                printf("Error: Número máximo de argumentos excedido\n");
                pthread_exit(NULL);
            }
        }
        i++;
        n++;
    }


    if (cuenta1 == -1){
        printf("Error: Falta indicar la cuenta\n");
        pthread_exit(NULL);
    }
    if (cuenta2 == -1 && strncmp(op.operacion, "TRASPASAR", 9) == 0){
        printf("Error: Falta indicar la cuenta destinataria\n");
        pthread_exit(NULL);
    }
    if ((cantidad == -1 && strncmp(op.operacion, "CREAR", 5) != 0) &&
        (cantidad == -1 && strncmp(op.operacion, "SALDO", 5) != 0) ){
        printf("Error: Falta indicar la cantidad\n");
        pthread_exit(NULL);
    }

    printf("Terminando hilo. Operación establecida: %s. \nNum cuenta1:%d. \nNum cuenta2:%d\n", op.operacion, op.num_cuenta1, op.num_cuenta2);
    printf("Saldo de la cuenta: %d\n\n", op.cantidad);
    return op;
}