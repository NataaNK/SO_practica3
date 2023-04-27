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
    char operacion;
    int num_cuenta;
    int cantidad;
}operacion;
 
int main (int argc, const char * argv[] ) {
    //./bank <nombre fichero> <num cajeros> <num trabajadores> <max cuentas>
    // <tam buff>

    // Comprobamos que el número de argumentos es correcto
    if (argc != 6){
        perror("Error: Número de argumentos incorrecto\n");
    }

    // Abrimos el fichero de entrada (operaciones)
    int fd;
    if (fd = open(argv[1], O_RDONLY) < 0){
        perror("Error: No se ha podido abrir el fichero\n");
    }
    
    // El segundo argumento nos especifica el número de cajeros (hilos productores)
    int num_cajeros;
    if (num_cajeros = atoi(argv[2]) <= 0){
        perror("Error: El número de cajeros debe ser mayor que 0\n");
    }

    // El tercer argumento representa el número de trabajadores del banco (hilos consumidores)
    int num_empleados;
    if (num_empleados = atoi(argv[3]) <= 0){
        perror("Error: El número de empleados debe ser mayor que 0\n");
    }

    // El cuarto argumento representa el número máximo de cuentas que puede haber en el banco
    int max_cuentas;
    if (max_cuentas = atoi(argv[4]) <= 0){
        perror("Error: El número máximo de cuentas debe ser mayor que 0\n");
    }
    
    // El quinto argumento representa el tamaño de la cola circular
    // sobre el que se irán almacenando las operaciones
    int tam_buff;
    if (tam_buff = atoi(argv[5]) <= 0){
        perror("Error: El tamaño del buffer debe ser mayor que 0\n");
    }
    
    queue *cola;
    cola = queue_init(tam_buff);
    
    // Leemos el fichero de entrada
    int bytes;
    // Diferenciador entre fichero vacío y fichero leído
    int vacio = 1;
    char buf[1];
    int seguir_comprobando = 1;
    int contador = 0;
    char num_operaciones_char[3];
    while ((bytes = read(fd, buf, 1)) > 0){ 
        // Si estamos al principio del fichero, leemos primero 
        // el número de operaciones que se realizarán hasta encontrar 
        // un salto de línea
        if (contador == 0){
            if(buf[0] == '\n'){
                int num_operaciones;
                num_operaciones = atoi(num_operaciones_char);
                if (num_operaciones > 200){
                    printf("No se pueden superar las 200 operaciones máximas");
                    close(fd);
                    exit(-1);
                }
                contador ++;
            }
            else{
                strncat(num_operaciones_char, buf, 1);
            }
        }
        else{
            if (strcmp(buf[0], "C") == 0){
            char buf_c[4];
            if(read(fd, buf_c, 4) < 0){
                perror("Error en la lectura del fichero");
            }
            if (strcmp(buf_c, "REAR") == 0){
                operacion op;
                op.num_operacion = num_operaciones;
            }
        }
        }


        // ********************************CÓDIGO DEL MYENV************************************
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
    
    if (bytes < 0){
        perror("Error: No se ha podido leer el fichero de entrada");
        close(fd);
    }

    return 0;
}