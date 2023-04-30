#include "queue.h"
#include <stdio.h>

int main (){
    queue *cola;
    cola = queue_init(10);
    printf("%d\n",cola->max_size);
}