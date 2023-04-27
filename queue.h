#ifndef HEADER_FILE
#define HEADER_FILE


typedef struct element {
	// Define the struct yourself
	int *param1;
}element;

typedef struct queue {
	// Define the struct yourself
	int param1;
	element **element;
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
