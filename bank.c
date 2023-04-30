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

// Constantes
#define MAX_OPERACIONES 200
#define MAX_DIGITOS_CANTIDAD_TRANSACCION 10


/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */

// Estructura de las operaciones
typedef struct operacion{
    int num_operacion;
    char operacion[9];
    int num_cuenta1;
    int num_cuenta2;
    int cantidad;
}operacion_t;

typedef struct insertar_elem{
    char *operacion_str;
    operacion_t operacion;
    int digit_max_cuentas;
}insertar_elem_t;

// Prototipos
operacion_t crear_elemento_operacion(char *operacion_str, operacion_t op, int digit_max_cuentas);

int insertar_elemento_en_cola(insertar_elem_t *op);

 
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

    // Leemos el fichero de entrada
    int bytes;
    // Diferenciador entre fichero vacío y fichero leído
    int vacio = 1;
    char buf[1];
    int contador = 0;
    char num_operaciones_char[3] = "";
    int num_operaciones;
    int primer_char = 1;
    // Lista que contiene las operaciones en formato string
    char **list_client_ops;

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

    // Una vez leída la entrada, establecemos la cola circular que compartiran
    // los hilos
    //queue *cola;
    //cola = queue_init(tam_buff);
    // Creamos el cajero (hilo productor) y le pasamos una operación,
    // deben leer la operación indicada en client_numop
    int client_numop=1;
    pthread_t th[num_cajeros];
    while (client_numop <= num_operaciones){
        int i;
        for(i=0; i<num_cajeros; i++){
            printf("Creando hilo %d\n", i);
            operacion_t op;
            // Establecemos el número de operación correspondiente
            op.num_operacion = client_numop;
            insertar_elem_t op_y_numop;
            op_y_numop.operacion_str = list_client_ops[client_numop - 1];
            op_y_numop.operacion = op;
            op_y_numop.digit_max_cuentas = digitos_max_cuentas;
            client_numop++;
            pthread_create(&th[i], NULL, insertar_elemento_en_cola, &op_y_numop);
            pthread_join(th[i], NULL);
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

int insertar_elemento_en_cola(insertar_elem_t *op){
    printf("entro\n");
    printf("hilo: %ld\n", pthread_self());
    crear_elemento_operacion(op->operacion_str, op->operacion, op->digit_max_cuentas);
    // Lo escribimos en la cola, sin que el resto de hilos puedan escribir y en ordern

    /*  // En caso de ser la operación CREAR, comprobamos que no exista ya
    // dicha cuenta
    else if(cantidad == -1 && strncmp(op.operacion, "CREAR", 5) == 0){
        int j;
        for (j=0;j<client_numop;j++){
            if (list_operaciones[j].operacion == op.operacion
                && list_operaciones[j].num_cuenta1 == op.num_cuenta1){
                // Quizás hay que bloquear únicamente los HILOS <---------------------------
                printf("Error: La cuenta ya existe\n");
                close(fd);
                exit(-1);
            }
        }   
    }*/
    sleep(1);
    return 0;
}

operacion_t crear_elemento_operacion(char *operacion_str, operacion_t op, int digit_max_cuentas){
    // Buscamos el número de cuenta y cantidad                        
    int read_arg1 = 0;
    int read_arg2 = 0;
    int read_arg3 = 0;
    char cuenta1_char[digit_max_cuentas];
    char cuenta2_char[digit_max_cuentas];
    char cantidad_char[MAX_DIGITOS_CANTIDAD_TRANSACCION]; 
    int cuenta1 = -1; 
    int cuenta2 = -1;
    int cantidad = -1;
    int i;
    int longitud = strlen(operacion_str);
    int digitos = 1;
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
        exit(-1);
    }

    int n = 0;
    // Buscamos los argumentos
    while (i<longitud){
        if ((read_arg1 == 1) || (read_arg2 == 1)){
            // Reiniciamos la posición del char dentro del argumento
            n = 0;
        }
        printf("analizando caracter: %c\n", operacion_str[i]);
        printf("nose que poner: %d\n", operacion_str[i] != ' ');
        if ((operacion_str[i] != ' ') && (read_arg1 == 0)){
            cuenta1_char[n] = operacion_str[i];
            n++;
            printf("cuenta1_char: %s\n", cuenta1_char);
            if ((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                read_arg1 = 1;
                cuenta1 = atoi(cuenta1_char);
                op.num_cuenta1 = cuenta1;
            }
        }
        else if((operacion_str[i] != ' ') && (read_arg2 == 0)){
            if (strncmp(op.operacion, "TRASPASAR", 9) != 0){
                if (digitos <= MAX_DIGITOS_CANTIDAD_TRANSACCION){
                    cantidad_char[n] = operacion_str[i];
                    n++;
                }
                else{
                    printf("Error: No se admite esa cantidad, debe tener 10 dígitos como máximo");
                    exit(-1);
                }
                digitos++;
                if ((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                    read_arg2 = 1;
                    cantidad = atoi(cantidad_char);
                    op.cantidad = cantidad;
                }
            }
            else if(strncmp(op.operacion, "CREAR", 5) == 0 || 
                    strncmp(op.operacion, "SALDO", 5) == 0 ){

                printf("Error: Número máximo de argumentos excedido\n");
                exit(-1);
            }
            else{
                // Si es la operación TRASPASAR buscamos cuenta2
                cuenta2_char[n] = operacion_str[i];
                n++;
                if ((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                    read_arg2 = 1;
                    cuenta2 = atoi(cuenta2_char);
                    op.num_cuenta2 = cuenta2;
                }
            }
        }
        // Si la operación es TRASPASAR buscamos la cantidad, si otra operación entra en el if
        // es que tiene una operación de más
        else if((operacion_str[i] != ' ') && (read_arg3 == 0)){
            if (strncmp(op.operacion, "TRASPASAR", 9) == 0){
                if (digitos <= MAX_DIGITOS_CANTIDAD_TRANSACCION){
                    cantidad_char[n] = operacion_str[i];
                    n++;
                }
                else{
                    printf("Error: No se admite esa cantidad, debe tener 10 dígitos como máximo");
                    exit(-1);
                }     
                digitos++;
                if((i+1 == longitud) || (operacion_str[i+1] == ' ')){
                    read_arg3 = 1;
                    cantidad = atoi(cantidad_char);
                    op.cantidad = cantidad;
                }
            }
            else{
                printf("Error: Número máximo de argumentos excedido\n");
                exit(-1);
            }
        }
        i++;
    }


    if (cuenta1 == -1){
        printf("Error: Falta indicar la cuenta\n");
        exit(-1);
    }
    if (cuenta2 == -1 && strncmp(op.operacion, "TRASPASAR", 9) == 0){
        printf("Error: Falta indicar la cuenta destinataria\n");
        exit(-1);
    }
    if ((cantidad == -1 && strncmp(op.operacion, "CREAR", 5) != 0) &&
        (cantidad == -1 && strncmp(op.operacion, "SALDO", 5) != 0) ){
        printf("Error: Falta indicar la cantidad\n");
        exit(-1);
    }

    printf("Terminando hilo. Operación establecida: %s. Num cuenta:%d\n", op.operacion, op.num_cuenta1);
    printf("Saldo de la cuenta: %d\n", op.cantidad);
    return op;
}
