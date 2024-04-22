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
#include <stdbool.h>

/*
constantes y variables globales 
*/
#define NUM_PRODUCTS 5
queue* buffer;  // Cola circular para las operaciones
pthread_mutex_t mutex;  // Mutex para controlar el acceso a la cola
pthread_cond_t can_produce;  // Condición para verificar si se puede producir (cola no llena)
pthread_cond_t can_consume;  // Condición para verificar si se puede consumir (cola no vacía)
/*La siguiente flag sirve para verificar que todos los hilos productores han terminado 
de producir*/
int producers_finished = false;

/*
La siguiente estructura sirva para almacenar cada operación.
*/
typedef struct {
    int product_id;
    int op_type;  // 0 for PURCHASE, 1 for SALE
    int units;
} operation;

/*
La siguiente estructura sirve para pasar argumentos a los hilos productores.
start_idx y end_idx indican el rango de operaciones que debe procesar el hilo.
Es decir, si hay 10 operaciones y 2 hilos productores, el primer hilo procesará
las operaciones 0-4 y el segundo hilo procesará las operaciones 5-9.
*/
typedef struct {
    operation* ops;
    int start_idx;
    int end_idx;
} producer_arg;

/*
La siguiente estructura sirve para que los hilos consumidores lleven
la cuenta del profit y la cantidad de productos en stock.
*/
typedef struct {
    int profit; // Profit acumulado por el consumidor
    int* stock; // Stock acumulado por producto
} consumer_result;

/*Declaraciones*/
operation* load_operations(const char* filename, int* total_ops);
void* producer(void* arg);
void* consumer(void* arg);

int main (int argc, const char * argv[]){
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

  pthread_t producers[num_producers], consumers[num_consumers];
  producer_arg p_args[num_producers];
  int ops_per_prod_thread = total_ops / num_producers;

  buffer = queue_init(buffer_size);
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&can_produce, NULL);
  pthread_cond_init(&can_consume, NULL);

  /*Crear hilos productores*/
  for (int i = 0; i < num_producers; i++) {
      p_args[i].ops = ops;
      p_args[i].start_idx = i * ops_per_prod_thread;
      p_args[i].end_idx = (i + 1) * ops_per_prod_thread - 1;
      /*El último hilo tendrá las operaciones restantes*/
      if (i == num_producers - 1)
          p_args[i].end_idx = total_ops - 1;

      pthread_create(&producers[i], NULL, (void *) &producer, (void*) &p_args[i]);
  }

  /*Crear hilos consumidores*/
  for (int i = 0; i < num_consumers; i++) {
      pthread_create(&consumers[i], NULL, (void *) &consumer, NULL);
  }

  /*Esperar a que los hilos productores terminen*/
  for (int i = 0; i < num_producers; i++) {
      pthread_join(producers[i], NULL);
  }
  producers_finished = true;  // Indicar que los productores han terminado
  printf("flag EN MAIN: %d\n", producers_finished);

  /*Esperar a que los hilos consumidores terminen y recoger resultados*/
  consumer_result* result;
  for (int i = 0; i < num_consumers; i++) {
      pthread_join(consumers[i], (void*) &result);
      profits += result->profit;
      for (int j = 0; j < NUM_PRODUCTS; j++) {
          product_stock[j] += result->stock[j];
      }
      free(result->stock);
  }

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

void* producer(void* arg) {
    producer_arg* p_arg = (producer_arg*) arg;

    for (int i = p_arg->start_idx; i <= p_arg->end_idx; i++) { 
        element *elem = malloc(sizeof(element));
        elem->product_id = p_arg->ops[i].product_id;
        elem->op = p_arg->ops[i].op_type;  // 0 for PURCHASE, 1 for SALE
        elem->units = p_arg->ops[i].units;

        pthread_mutex_lock(&mutex);
        while (queue_full(buffer)) {
            pthread_cond_wait(&can_produce, &mutex);
        }
        queue_put(buffer, elem);
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void* consumer(void* arg) {
    consumer_result result;
    /*
    inicializamos todos los resultados a 0
    */
    result.profit = 0;
    result.stock = malloc(sizeof(int) * NUM_PRODUCTS);
    for(int i = 0; i < NUM_PRODUCTS; i++) {
        result.stock[i] = 0;
    }
    /*
    Los hilos produtores han terminado de producir cuando la variable global
    producers_finished es verdadera y la cola está vacía.
    */
    int caca = 0;
    while (!producers_finished || !queue_empty(buffer)) { 
      printf("flag: %d\n", producers_finished);
      printf("caca: %d\n", caca);
      caca++;
      pthread_mutex_lock(&mutex);
      while (queue_empty(buffer)) {
          pthread_cond_wait(&can_consume, &mutex);
      }
      element* task = queue_get(buffer);
      pthread_cond_signal(&can_produce);
      pthread_mutex_unlock(&mutex);

      // Procesar la operación
      if (task->op == 0) {  // PURCHASE
          if (task->product_id == 1) {
              result.stock[0] += task->units;
              result.profit -= task->units * 2;
          } else if (task->product_id == 2) {
              result.stock[1] += task->units;
              result.profit -= task->units * 5;
          } else if (task->product_id == 3) {
              result.stock[2] += task->units;
              result.profit -= task->units * 15;
          } else if (task->product_id == 4) {
              result.stock[3] += task->units;
              result.profit -= task->units * 25;
          } else if (task->product_id == 5) {
              result.stock[4] += task->units;
              result.profit -= task->units * 100;
          }
      } else if (task->op == 1) {  // SALE
          if (task->product_id == 1) {
              result.stock[0] -= task->units;
              result.profit += task->units * 3;
          } else if (task->product_id == 2) {
              result.stock[1] -= task->units;
              result.profit += task->units * 10;
          } else if (task->product_id == 3) {
              result.stock[2] -= task->units;
              result.profit += task->units * 20;
          } else if (task->product_id == 4) {
              result.stock[3] -= task->units;
              result.profit += task->units * 40;
          } else if (task->product_id == 5) {
              result.stock[4] -= task->units;
              result.profit += task->units * 125;
      }
    }
  }
  pthread_exit(&result);
}
