//SSOO-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"


//To create a queue

queue* queue_init(int size){
    queue *q = (queue *)malloc(sizeof(queue));
    q->max_size = size;
    q->size = 0;
    q->element = (operacion_t *)malloc(sizeof(operacion_t) * size);
    return q;
}


// To Enqueue an element
int queue_put(queue *q, operacion_t elem) {
	q->element[q->size] = elem;
	q->size++;
	return 0;
}


// To Dequeue an element.
operacion_t queue_get(queue *q) {
	operacion_t element;
	element = q->element[0];
	// Actualizamos los índices
	int i;
	for (i=0;i<q->size-1;i++){
		q->element[i] = q->element[i+1];
	}
	q->size--;
	return element;
}


//To check queue state
int queue_empty(queue *q){
	if (q->size == 0){
		return 1;
	}
	else{
		return 0;
	}
}

int queue_full(queue *q){
	if (q->size == q->max_size){
		return 1;
	}
	else{
		return 0;
	}
}

//To destroy the queue and free the resources
int queue_destroy(queue *q){
	q->element = NULL;
	q->max_size = 0;
	q->size = 0;
	free(q);
	return 0;
}