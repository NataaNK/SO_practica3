/* 
SSOO-P3 23/24 

    - ALberto Penas Díaz        - 100471939
    - Natalia Rodríguez Navarro - 100471976

*/

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
queue *buffer;
pthread_mutex_t mutex;
pthread_cond_t can_produce;
pthread_cond_t can_consume;

/*
La siguiente estructura sirva para almacenar cada operación.
*/
typedef struct
{
    int product_id;
    int op_type; /*0 for PURCHASE, 1 for SALE*/
    int units;
} operation;

/*
La siguiente estructura sirve para pasar argumentos a los hilos productores.
start_idx y end_idx indican el rango de operaciones que debe procesar el hilo.
Es decir, si hay 10 operaciones y 2 hilos productores, el primer hilo procesará
las operaciones 0-4 y el segundo hilo procesará las operaciones 5-9.
*/
typedef struct
{
    operation *ops;
    int start_idx;
    int end_idx;
} producer_arg;

/*
La siguiente estructura sirve para que los hilos consumidores lleven
la cuenta del profit y la cantidad de productos en stock.
*/
typedef struct
{
    int profit;
    int *stock;
} consumer_result;

/*Declaraciones*/
operation *load_operations(const char *filename, int *total_ops);
void *producer(void *arg);
void *consumer(int *iterations);
void process_task(consumer_result *result, element *task);

int main(int argc, const char *argv[])
{
    int profits = 0;
    int product_stock[5] = {0};

    /*
    Primeramente, comprobaremos los argumentos de entrada.
    */
    if (argc != 5)
    {
        printf("[ERROR] Hacen falta 4 argumentos\n");
        return -1;
    }

    /*
    El primer argumento es el fichero de entrada de la función.
    */
    int total_ops = 0;
    operation *ops = load_operations(argv[1], &total_ops);
    if (ops == NULL)
    {
        printf("Failed to load operations from file '%s'\n", argv[1]);
        return -1;
    }

    /*
    El siguiente argumento representa el número de hilos productores.
    */
    int num_producers = atoi(argv[2]);
    if (num_producers < 1)
    {
        printf("[ERROR] El número de productores debe ser un número mayor que 0\n");
        return -1;
    }

    /*
    El siguiente argumento representa el número de hilos consumidores.
    */
    int num_consumers = atoi(argv[3]);
    if (num_consumers < 1)
    {
        printf("[ERROR] El número de consumidores debe ser un número mayor que 0\n");
        return -1;
    }

    /*
    El último argumento representa el tamaño del buffer.
    */
    int buffer_size = atoi(argv[4]);
    if (buffer_size < 1)
    {
        printf("[ERROR] El tamaño del buffer debe ser un número mayor que 0\n");
        return -1;
    }

    pthread_t producers[num_producers], consumers[num_consumers];
    producer_arg p_args[num_producers];
    int ops_per_prod_thread = total_ops / num_producers;
    int ops_per_cons_thread = total_ops / num_consumers;
    int ops_last_const_thread = total_ops % num_consumers;

    buffer = queue_init(buffer_size);
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&can_produce, NULL);
    pthread_cond_init(&can_consume, NULL);

    /*Crear hilos productores*/
    for (int i = 0; i < num_producers; i++)
    {
        p_args[i].ops = ops;
        p_args[i].start_idx = i * ops_per_prod_thread;
        p_args[i].end_idx = (i + 1) * ops_per_prod_thread - 1;
        /*El último hilo tendrá las operaciones restantes*/
        if (i == num_producers - 1)
            p_args[i].end_idx = total_ops - 1;

        pthread_create(&producers[i], NULL, (void *)&producer, (void *)&p_args[i]);
    }

    /*Crear hilos consumidores*/
    for (int i = 0; i < num_consumers; i++)
    {
        if (i == num_consumers - 1 && ops_last_const_thread != 0)
        {
            ops_last_const_thread = ops_per_cons_thread + ops_last_const_thread;
            pthread_create(&consumers[i], NULL, (void *)&consumer, &ops_last_const_thread);
        }
        else
        {
            pthread_create(&consumers[i], NULL, (void *)&consumer, &ops_per_cons_thread);
        }
    }

    /*Esperar a que los hilos productores terminen*/
    for (int i = 0; i < num_producers; i++)
    {
        pthread_join(producers[i], NULL);
    }

    /*Esperar a que los hilos consumidores terminen*/
    consumer_result *results[num_consumers];
    for (int i = 0; i < num_consumers; i++)
    {
        pthread_join(consumers[i], (void **)&results[i]);
    }

    for (int i = 0; i < num_consumers; i++)
    {
        consumer_result *result = results[i];
        profits += result->profit;
        for (int j = 0; j < NUM_PRODUCTS; j++)
        {
            product_stock[j] += result->stock[j];
        }

        free(results[i]->stock);
        free(result);
    }
    /*
    Comprobarmos que el stock no sea negativo de ninguno de los productos.
    */
    for (int i = 0; i < NUM_PRODUCTS; i++)
    {
        if (product_stock[i] < 0)
        {
            printf("[ERROR] El stock del producto %d no puede ser negativo\n", i + 1);
            return -1;
        }
    }

    printf("Total: %d euros\n", profits);
    printf("Stock:\n");
    printf("  Product 1: %d\n", product_stock[0]);
    printf("  Product 2: %d\n", product_stock[1]);
    printf("  Product 3: %d\n", product_stock[2]);
    printf("  Product 4: %d\n", product_stock[3]);
    printf("  Product 5: %d\n", product_stock[4]);

    free(ops);
    queue_destroy(buffer);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}

operation *load_operations(const char *filename, int *total_ops)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        perror("Error opening file");
        return NULL;
    }

    int max_operations;
    if (fscanf(fp, "%d\n", &max_operations) != 1)
    {
        fprintf(stderr, "Failed to read the number of operations.\n");
        fclose(fp);
        return NULL;
    }

    operation *operations = malloc(max_operations * sizeof(operation));

    char line[256];
    int index = 0;
    int product_id, units;
    char type[10];

    while (fgets(line, sizeof(line), fp) && index < max_operations)
    {
        if (sscanf(line, "%d %s %d", &product_id, type, &units) == 3)
        {
            operations[index].product_id = product_id;
            operations[index].units = units;
            if (strcmp(type, "PURCHASE") == 0)
                operations[index].op_type = 0;
            else if (strcmp(type, "SALE") == 0)
                operations[index].op_type = 1;
            else
            {
                printf("Unknown operation type '%s' at index %d\n", type, index);
                free(operations);
                fclose(fp);
                return NULL;
            }
            index++;
        }
    }

    if (index != max_operations)
    {
        printf("File contains fewer operations (%d) than specified (%d).\n", index, max_operations);
        free(operations);
        fclose(fp);
        return NULL;
    }

    *total_ops = max_operations;
    fclose(fp);
    return operations;
}

void *producer(void *arg)
{
    producer_arg *p_arg = (producer_arg *)arg;

    for (int i = p_arg->start_idx; i <= p_arg->end_idx; i++)
    {
        element *elem = malloc(sizeof(element));
        elem->product_id = p_arg->ops[i].product_id;
        elem->op = p_arg->ops[i].op_type;
        elem->units = p_arg->ops[i].units;

        pthread_mutex_lock(&mutex);
        while (queue_full(buffer))
        {
            pthread_cond_wait(&can_produce, &mutex);
        }
        queue_put(buffer, elem);
        /*
        Aquí podemos liberar el elemento, ya que la cola ha hecho una copia de él.
        */
        free(elem);
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void *consumer(int *iterations)
{
    consumer_result *result = malloc(sizeof(consumer_result));

    result->profit = 0;
    result->stock = malloc(sizeof(int) * NUM_PRODUCTS);

    for (int i = 0; i < NUM_PRODUCTS; i++)
    {
        result->stock[i] = 0;
    }

    for (int i = 0; i < *iterations; i++)
    {
        pthread_mutex_lock(&mutex);
        while (queue_empty(buffer))
        {
            pthread_cond_wait(&can_consume, &mutex);
        }
        element *task = queue_get(buffer);
        process_task(result, task);
        free(task);
        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(result);
}

void process_task(consumer_result *result, element *task)
{
    int price[] = {2, 5, 15, 25, 100};
    int sale_price[] = {3, 10, 20, 40, 125};

    int idx = task->product_id - 1;

    if (task->op == 0)
    { /*PURCHASE*/
        result->stock[idx] += task->units;
        result->profit -= task->units * price[idx];
    }
    else if (task->op == 1)
    { /*SALE*/
        result->stock[idx] -= task->units;
        result->profit += task->units * sale_price[idx];
    }
}
