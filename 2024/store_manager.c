//SSOO-P3 23/24

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

operation* load_operations(const char* filename, int* total_ops);

/*
La siguiente estructura sirva para almacenar cada operación.
*/
typedef struct {
    int product_id;
    int op_type;  // 0 for PURCHASE, 1 for SALE
    int units;
} operation;


int main (int argc, const char * argv[])
{
  int profits = 0;
  int product_stock [5] = {0};

  /*
  Primeramente, comprobaremos los argumentos de entrada. 
  */
  if (argc != 5) {
    printf("[ERROR] Hacen falta 4 argumentos\n");
    return -1;
  }

  /*
  El primer argumento es el fichero de entrada de la función.
  */
  int total_ops = 0;
  operation* ops = load_operations(argv[1], &total_ops);
  if (ops == NULL) {
      printf("Failed to load operations from file '%s'\n", argv[1]);
      return -1;
  }
  printf("Total operations loaded: %d\n", total_ops);
    for (int i = 0; i < total_ops; i++) {
        printf("Operation %d: Product %d, Type %s, Units %d\n",
               i+1, ops[i].product_id,
               ops[i].op_type == 0 ? "PURCHASE" : "SALE",
               ops[i].units);
    }

  /*
  El siguiente argumento representa el número de hilos productores.
  */
  int num_producers = atoi(argv[2]);
  if (num_producers < 1) {
    printf("[ERROR] El número de productores debe ser un número mayor que 0\n");
    return -1;
  }

  /*
  El siguiente argumento representa el número de hilos consumidores.
  */
  int num_consumers = atoi(argv[3]);
  if (num_consumers < 1) {
    printf("[ERROR] El número de consumidores debe ser un número mayor que 0\n");
    return -1;
  }

  /*
  El último argumento representa el tamaño del buffer.
  */
  int buffer_size = atoi(argv[4]);
  if (buffer_size < 1) {
    printf("[ERROR] El tamaño del buffer debe ser un número mayor que 0\n");
    return -1;
  }

  /*
  Una vez comprobados los argumentos, pasaremos el fichero principal a memoria
  para poder leerlo posteriormente de forma paralela.
  */



  // Output
  printf("Total: %d euros\n", profits);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  free(ops); 
  return 0;
}
operation* load_operations(const char* filename, int* total_ops) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return NULL;
    }

    char buffer[1024];  // Buffer to store file contents temporarily
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Error reading file");
        close(fd);
        return NULL;
    }
    buffer[bytes_read] = '\0';  // Null terminate the string

    int max_operations;
    char* buf_ptr = buffer;
    sscanf(buf_ptr, "%d\n", &max_operations);
    buf_ptr = strchr(buf_ptr, '\n') + 1;  // Move pointer to next line

    operation* operations = malloc(max_operations * sizeof(operation));
    if (operations == NULL) {
        perror("Failed to allocate memory for operations");
        close(fd);
        return NULL;
    }

    int index = 0;
    int product_id, units;
    char type[10];

    while (sscanf(buf_ptr, "%d %s %d\n", &product_id, type, &units) == 3 && index < max_operations) {
        operations[index].product_id = product_id;
        operations[index].units = units;
        if (strcmp(type, "PURCHASE") == 0)
            operations[index].op_type = 0;
        else if (strcmp(type, "SALE") == 0)
            operations[index].op_type = 1;
        else {
            fprintf(stderr, "Unknown operation type '%s' at index %d\n", type, index);
            free(operations);
            close(fd);
            return NULL;
        }
        buf_ptr = strchr(buf_ptr, '\n') + 1;  // Move to next line
        index++;
    }

    *total_ops = index;  // Set the actual number of operations read
    close(fd);
    return operations;
}


