//SSOO-P3 23/24

#ifndef HEADER_FILE
#define HEADER_FILE


typedef struct element {
  int product_id; //Product identifier
  int op;         //Operation
  int units;      //Product units
} element;

// Definición de la estructura para la cola
typedef struct queue {
  element* elements; // Array dinámico para almacenar los elementos
  int capacity; // Capacidad máxima de la cola
  int front; // Índice al primer elemento de la cola
  int rear; // Índice al último elemento de la cola
  int size; // Número de elementos en la cola
} queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
