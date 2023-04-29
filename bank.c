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
}operacion;

int introducir_en_list_ops(int fd, operacion op, operacion *list_operaciones, 
                           int contador, int max_cuentas){
    // Establecemosel número de operación
    op.num_operacion = contador;
    // Buscamos el número de cuenta y cantidad                        
    char buf[1];
    int bytes;
    int read_arg1 = 0;
    int read_arg2 = 0;
    int read_arg3 = 0;
    char cuenta1_char[sizeof(max_cuentas)/sizeof(int)] = "";
    char cuenta2_char[sizeof(max_cuentas)/sizeof(int)] = "";
    // MALLOC??? <------------------------------------------------------------
    char cantidad_char[10] = ""; 
    int cuenta1 = -1; 
    int cuenta2 = -1;
    int cantidad = -1;
    int fin_de_linea = 0;
    while ((fin_de_linea == 0) && (bytes = read(fd, buf, 1) > 0) 
            && (strncmp(buf, "\n", 1) != 0)){
        if ((strncmp(buf, " ", 1) != 0) && (read_arg1 == 0)){
            read_arg1 = 1;
            strncat(cuenta1_char, buf, 1);
            while ((bytes = read(fd, buf, 1) > 0) && (strncmp(buf, " ", 1) != 0)
                    && (strncmp(buf, "\n", 1) != 0)){
                strncat(cuenta1_char, buf, 1);
            }
            cuenta1 = atoi(cuenta1_char);
            op.num_cuenta1 = cuenta1;
            if (strncmp(buf, "\n", 1) == 0){
                fin_de_linea = 1;
            }      
        }
        else if ((strncmp(buf, " ", 1) != 0) && (read_arg2 == 0)){
            read_arg2 = 1;
            if (strncmp(op.operacion, "TRASPASAR", 9) != 0){
                strncat(cantidad_char, buf, 1);
                while ((bytes = read(fd, buf, 1) > 0) && (strncmp(buf, " ", 1) != 0)
                        && (strncmp(buf, "\n", 1) != 0)){
                    strncat(cantidad_char, buf, 1);
                }
                cantidad = atoi(cantidad_char);
                op.cantidad = cantidad;
                if (strncmp(buf, "\n", 1) == 0){
                    fin_de_linea = 1;
                }
                
            }
            else if(strncmp(op.operacion, "CREAR", 5) == 0 || 
                    strncmp(op.operacion, "SALDO", 5) == 0 ){
                printf("Error: número máximo de argumentos excedido\n");
                close(fd);
                exit(-1);
            }
            else{
                // Si es la operación TRASPASAR buscamos cuenta2
                strncat(cuenta2_char, buf, 1);
                while ((bytes = read(fd, buf, 1) > 0) && (strncmp(buf, " ", 1) != 0)
                        && (strncmp(buf, "\n", 1) != 0)){
                    strncat(cuenta2_char, buf, 1);
                }
                cuenta2 = atoi(cuenta2_char);
                op.num_cuenta2 = cuenta2;
                if (strncmp(buf, "\n", 1) == 0){
                    fin_de_linea = 1;
                }
            }
        }
        // Si la operación es TRASPASAR buscamos la cantidad, si otra operación entra en el if
        // es que tiene una operación de más
        else if((strncmp(buf, " ", 1) != 0) && (read_arg3 == 0)){
            read_arg3 = 1;
            if (strncmp(op.operacion, "TRASPASAR", 9) == 0){
                strncat(cantidad_char, buf, 1);
                while ((bytes = read(fd, buf, 1) > 0) && (strncmp(buf, " ", 1) != 0)
                        && (strncmp(buf, "\n", 1) != 0)){
                    strncat(cantidad_char, buf, 1);
                }
                cantidad = atoi(cantidad_char);
                op.cantidad = cantidad;
                if (strncmp(buf, "\n", 1) == 0){
                    fin_de_linea = 1;
                }
                
            }
            else{
                printf("Error: número máximo de argumentos excedido\n");
                close(fd);
                exit(-1);
            }
        }
    }

    if (bytes < 0){
        perror("Error de lectura");
        close(fd);
        exit(-1);
    }
    if (cuenta1 == -1){
        // <----------------------------------------------------------------¿BLOQUEAR TODO?
        printf("Error: Falta indicar la cuenta\n");
        close(fd);
        exit(-1);
    }
    if (cuenta2 == -1 && strncmp(op.operacion, "TRASPASAR", 9) == 0){
        printf("Error: Falta indicar la cuenta destinataria\n");
        close(fd);
        exit(-1);
    }
    if ((cantidad == -1 && strncmp(op.operacion, "CREAR", 5) != 0) &&
        (cantidad == -1 && strncmp(op.operacion, "SALDO", 5) != 0) ){
        // <----------------------------------------------------------------¿BLOQUEAR TODO?
        printf("Error: Falta indicar la cantidad\n");
        close(fd);
        exit(-1);
    }
    // En caso de ser la operación CREAR, comprobamos que no exista ya
    // dicha cuenta
    else if(cantidad == -1 && strncmp(op.operacion, "CREAR", 5) == 0){
        int j;
        for (j=0;j<contador;j++){
            if (list_operaciones[j].operacion == op.operacion
                && list_operaciones[j].num_cuenta1 == op.num_cuenta1){
                // Quizás hay que bloquear únicamente los HILOS <---------------------------
                printf("Error: La cuenta ya existe\n");
                close(fd);
                exit(-1);
            }
        }   
    
    // Añadimos la operación a la lista de operaciones
    list_operaciones[contador - 1] = op;
    return 0;
    }
}
 
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
    if (num_cajeros = atoi(argv[2]) <= 0){
        printf("Error: El número de cajeros debe ser mayor que 0\n");
        return -1;
    }

    // El tercer argumento representa el número de trabajadores del banco (hilos consumidores)
    int num_empleados;
    if (num_empleados = atoi(argv[3]) <= 0){
        printf("Error: El número de empleados debe ser mayor que 0\n");
        return -1;
    }

    // El cuarto argumento representa el número máximo de cuentas que puede haber en el banco
    int max_cuentas;
    if (max_cuentas = atoi(argv[4]) <= 0){
        printf("Error: El número máximo de cuentas debe ser mayor que 0\n");
        return -1;
    }
    
    // El quinto argumento representa el tamaño de la cola circular
    // sobre el que se irán almacenando las operaciones
    int tam_buff;
    if (tam_buff = atoi(argv[5]) <= 0){
        printf("Error: El tamaño del buffer debe ser mayor que 0\n");
        return -1;
    }
    
    queue *cola;
    //cola = queue_init(tam_buff);

    // Leemos el fichero de entrada
    int bytes;
    // Diferenciador entre fichero vacío y fichero leído
    int vacio = 1;
    char buf[1];
    int contador = 0;
    // Hay que inicializar el contador de operaciones (char) a ""
    // para que, al transformarlo a int, lo detecte correctamente
    char num_operaciones_char[3] = "";
    int num_operaciones;
    // Estableceremos el espacio de la lista de operaciones al leer la
    // cantidad en el fichero
    operacion *list_client_ops;

    while ((bytes = read(fd, buf, 1)) > 0){ 
        vacio = 0;
        // printf("Leyendo: %c\n", buf[0]);
        // Si estamos al principio del fichero, leemos primero 
        // el número de operaciones que se realizarán hasta encontrar 
        // un salto de línea
        if (contador == 0){
            if(buf[0] == '\n'){
                num_operaciones = atoi(num_operaciones_char);
                // Creamos el array en el que se almacenarán las operaciones que 
                // el banco tiene que hacer (el tamaño del array es el número de operaciones)
                list_client_ops = (operacion*)malloc(sizeof(operacion)*num_operaciones);
                if (num_operaciones > 200){
                    printf("Error: No se pueden superar las 200 operaciones máximas");
                    close(fd);
                    exit(-1);
                }
                contador++;
            }
            else{
                strncat(num_operaciones_char, buf, 1);
            }
        }
        else{
            if (strncmp(buf, "C", 1) == 0){
                char buf_c[4];
                if(read(fd, buf_c, 4) < 0){
                    perror("Error en la lectura del fichero");
                    close(fd);
                    exit(-1);
                }
                if (strncmp(buf_c, "REAR", 4) == 0){
                    // Añadimos la nueva operación y buscamos con sus argumentos
                    operacion op;
                    strcpy(op.operacion, "CREAR");
                    introducir_en_list_ops(fd, op, list_client_ops, contador, max_cuentas);
                    // Comprobamos que no exista ya dicha cuenta
                    contador++;
                }
                else{
                    // Quizás hay que bloquear únicamente los HILOS <---------------------------
                    printf("Error: Operación incorrecta\n");
                    close(fd);
                    exit(-1);
                }
            }
            else if (strncmp(buf, "I", 1) == 0){
                char buf_i[7];
                if(read(fd, buf_i, 7) < 0){
                    perror("Error en la lectura del fichero");
                    close(fd);
                    exit(-1);
                }
                if (strncmp(buf_i, "NGRESAR", 7) == 0){
                    // Ahora, hay que buscar el número de cuenta e introducir la cantidad 
                    operacion op; 
                    strcpy(op.operacion, "INGRESAR");
                    introducir_en_list_ops(fd, op, list_client_ops, contador, max_cuentas);
                    // Comprobamos que no exista ya dicha cuenta
                    contador++;
                }
                else{
                    // Quizás hay que bloquear únicamente los HILOS <---------------------------
                    printf("Error: Operación incorrecta\n");
                    close(fd);
                    exit(-1);
                }
            }
            else if (strncmp(buf, "T", 1) == 0){
                char buf_t[8];
                if(read(fd, buf_t, 8) < 0){
                    perror("Error en la lectura del fichero");
                    close(fd);
                    exit(-1);
                }
                if (strncmp(buf_t, "RASPASAR", 8) == 0){
                    // Ahora, hay que buscar el número de cuenta e introducir la cantidad 
                    operacion op; 
                    strcpy(op.operacion, "TRASPASAR");
                    introducir_en_list_ops(fd, op, list_client_ops, contador, max_cuentas);
                    // Comprobamos que no exista ya dicha cuenta
                    contador++;
                }
                else{
                    // Quizás hay que bloquear únicamente los HILOS <---------------------------
                    printf("Error: Operación incorrecta\n");
                    close(fd);
                    exit(-1);
                }
            }
            else if (strncmp(buf, "R", 1) == 0){
                char buf_r[6];
                if(read(fd, buf_r, 6) < 0){
                    perror("Error en la lectura del fichero");
                    close(fd);
                    exit(-1);
                }
                if (strncmp(buf_r, "ETIRAR", 6) == 0){
                    // Ahora, hay que buscar el número de cuenta e introducir la cantidad 
                    operacion op; 
                    strcpy(op.operacion, "RETIRAR");
                    introducir_en_list_ops(fd, op, list_client_ops, contador, max_cuentas);
                    // Comprobamos que no exista ya dicha cuenta
                    contador++;
                }
                else{
                    // Quizás hay que bloquear únicamente los HILOS <---------------------------
                    printf("Error: Operación incorrecta\n");
                    close(fd);
                    exit(-1);
                }
            }
            else if (strncmp(buf, "S", 1) == 0){
                char buf_s[4];
                if(read(fd, buf_s, 4) < 0){
                    perror("Error en la lectura del fichero");
                    close(fd);
                    exit(-1);
                }
                if (strncmp(buf_s, "ALDO", 4) == 0){
                    // Ahora, hay que buscar el número de cuenta e introducir la cantidad 
                    operacion op; 
                    strcpy(op.operacion, "SALDO");
                    introducir_en_list_ops(fd, op, list_client_ops, contador, max_cuentas);
                    // Comprobamos que no exista ya dicha cuenta
                    contador++;
                }
                else{
                    // Quizás hay que bloquear únicamente los HILOS <---------------------------
                    printf("Error: Operación incorrecta\n");
                    close(fd);
                    exit(-1);
                }
            }
        }
    }

    if (bytes < 0){
        perror("Error: No se ha podido leer el fichero de entrada");
        close(fd);
        exit(-1);
    }
    
    if (vacio == 1){
        perror("Error: Fichero de entrada vacío");
        close(fd);
        exit(-1);
    }

    return 0;

        // ********************************CÓDIGO DEL MYENV************************************
        /*
        // Buscamos coincidencia en la línea
        if (seguir_comprobando == 1){

            if (buf[0] == variable[i]){
                i++;
                seguir_comprobando = 1;
            }
            else{
                i = 0;
                seguir_comprobando = 0;
            }
        }
        // Si hemos llegado al final de la línea, seguimos buscando
        // en la siguiente
        else if (buf[0] == '\n'){
            seguir_comprobando = 1;
        }
        // Como ya sabemos que no hay coincidencia en esa línea,
        // la pasamos sin comprobar hasta acabar la línea
        else{
            seguir_comprobando = 0;
        }        

        // Si han coincidido todas las letras querrá decir que hemos
        // encontrado la variable
        if (i == len){
            seguir_comprobando = 0;
            // Ya hemos acabado de escribir las entradas
            if (buf[0] == '\n'){
                // Añadimos el salto de línea
                strcat(buf_escritura, "\n");
                escrito = 1;
            }
            else{
                // En la primera iteración añadimos el nombre de la variable
                // y el '='
                if (primera_iteracion == 1){
                    primera_iteracion = 0; 
                    strcpy(buf_escritura, variable);
                }
                // En las siguientes añadimos el primer caracter del buf 
                // al destino, más un carácter nulo de terminación con 'strncat'
                else{
                    strncat(buf_escritura, buf, 1);
                }
            }
        }
    }
    */
    
    
}
