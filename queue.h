#ifndef QUEUE_H
#define QUEUE_H
#include <stdlib.h>

typedef struct queue q;

q *q_new(void);
int q_add(q *q, void *d);
void *q_pop(q *q);
size_t q_len(q *q);
void q_free(q *q);
#endif /* QUEUE_H */
