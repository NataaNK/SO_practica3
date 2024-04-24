#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define NUM_THREADS 300
#define NUM_OPERATIONS 50000000

int purchase_price[] = {2, 5, 15, 25, 100};
int sale_price[] = {3, 10, 20, 40, 125};
int stock[5] = {0};
int profit = 0;
int global_op_count = 0;
pthread_mutex_t lock;

typedef struct
{
    int thread_id;
    int num_operations;
} thread_data;

void *perform_operations(void *threadarg)
{
    thread_data *my_data;
    my_data = (thread_data *)threadarg;
    int tid = my_data->thread_id;
    int num_ops = my_data->num_operations;
    int local_profit = 0;
    int local_stock[5] = {0};

    for (int i = 0; i < num_ops; i++)
    {
        int product_id = rand() % 5;
        int operation_type = rand() % 100 < 15 ? 0 : 1; // 15% compra, 85% venta
        int units = rand() % 100 + 1;

        if (operation_type == 1 && local_stock[product_id] < units)
        {
            continue; // Ignora la operación de venta si no hay suficiente stock
        }

        // Actualizar el stock y la ganancia local
        if (operation_type == 0)
        {
            local_stock[product_id] += units;
            local_profit -= units * purchase_price[product_id];
        }
        else
        {
            local_stock[product_id] -= units;
            local_profit += units * sale_price[product_id];
        }

        // Bloquear para escribir al archivo
        pthread_mutex_lock(&lock);
        FILE *file = fopen("operations.txt", "a");
        if (file != NULL)
        {
            fprintf(file, "%d %s %d\n",
                    product_id + 1,
                    operation_type == 0 ? "PURCHASE" : "SALE",
                    units);
            fclose(file);
        }
        else
        {
            printf("Error opening file\n");
        }
        global_op_count++;
        pthread_mutex_unlock(&lock);
    }

    // Actualiza el stock global y la ganancia
    pthread_mutex_lock(&lock);
    for (int i = 0; i < 5; i++)
    {
        stock[i] += local_stock[i];
    }
    profit += local_profit;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];
    thread_data thread_data_array[NUM_THREADS];
    int rc;
    long t;

    pthread_mutex_init(&lock, NULL);

    // Limpia el archivo
    FILE *file = fopen("operations.txt", "w");
    if (file != NULL)
    {
        fclose(file);
    }

    for (t = 0; t < NUM_THREADS; t++)
    {
        thread_data_array[t].thread_id = t;
        thread_data_array[t].num_operations = NUM_OPERATIONS / NUM_THREADS;
        rc = pthread_create(&threads[t], NULL, perform_operations, (void *)&thread_data_array[t]);
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // Esperar a que todos los hilos terminen
    for (t = 0; t < NUM_THREADS; t++)
    {
        pthread_join(threads[t], NULL);
    }

    // Leer el contenido del archivo
    file = fopen("operations.txt", "r");
    if (file == NULL)
    {
        printf("Unable to open file to read contents.\n");
        pthread_exit(NULL);
    }
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, file);
    content[fsize] = '\0';
    fclose(file);

    // Reescribir el archivo con el total de operaciones al principio
    file = fopen("operations.txt", "w");
    if (file != NULL)
    {
        fprintf(file, "%d\n", global_op_count);
        fputs(content, file);
        fclose(file);
    }
    else
    {
        printf("Unable to open file to rewrite contents.\n");
    }
    free(content);

    printf("Resultados Esperados:\n");
    printf("Ganancia: %d\n", profit);
    for (int i = 0; i < 5; i++)
    {
        printf("Producto %d Stock: %d\n", i + 1, stock[i]);
    }

    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
}
