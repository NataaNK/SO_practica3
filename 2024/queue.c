//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

/*
En un buffer circular hay un puntero que apunta a la primera posición libre y 
otro que apunta a la primera posición ocupada. A medida que se escribe en el buffer
o se saca algo de él, los punteros se van moviendo. Cuando el puntero de escritura
alcanza el puntero de lectura, el buffer está lleno. Cuando el puntero de lectura
alcanza el puntero de escritura, el buffer está vacío.

Otra forma de saber si el buffer está lleno o vacío, es llevar un contador de elementos.
Un mutex es simplemente un semáforo de valor 1. Un mutex solo tiene en cuenta secciones críticas
de código, no de datos. 

La práctica está prácticamente solucionada (por lo menos la parte de productor-consumidor)
en las diapositivas de clase. 
*/

//To create a queue
queue* queue_init(int size) {
  queue *q = (queue *)malloc(sizeof(queue));
  q->elements = (element *)malloc(sizeof(element) * size);
  q->capacity = size;
  q->front = 0;
  q->rear = -1;
  q->size = 0;
  return q;
}

// Para enconlar un elemento
int queue_put(queue *q, element* elem) {
  if (q->size == q->capacity) {
    return -1; // La cola está llena
  }
  q->rear = (q->rear + 1) % q->capacity;
  q->elements[q->rear] = *elem;
  q->size++;
  return 0;
}

// Para desencolar un elemento.
element* queue_get(queue *q) {
  if (q->size == 0) {
    return NULL; // La cola está vacía
  }
  element* elem = &q->elements[q->front];
  q->front = (q->front + 1) % q->capacity;
  q->size--;
  return elem;
}

// Para comprobar el estado de la cola
int queue_empty(queue *q) {
  return q->size == 0;
}

int queue_full(queue *q) {
  return q->size == q->capacity;
}

// Para destruir la cola y liberar los recursos
int queue_destroy(queue *q) {
  free(q->elements);
  free(q);
  return 0;
}
