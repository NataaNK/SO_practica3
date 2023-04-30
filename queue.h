#ifndef HEADER_FILE
#define HEADER_FILE


// Estructura de las operaciones
typedef struct operacion {
    int num_operacion;
    char operacion[9];
    int num_cuenta1;
    int num_cuenta2;
    int cantidad;
}operacion_t;

typedef struct queue {
	int max_size;
	int size;
	operacion_t *element;
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put(queue *q, operacion_t elem);
operacion_t queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
