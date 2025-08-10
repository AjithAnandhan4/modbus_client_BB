#ifndef DATA_Q_H
#define DATA_Q_H

#include <stddef.h>

int data_queue_init(size_t capacity);
void data_queue_destroy(void);

int data_queue_enqueue(const char *topic, const char *payload);
int data_queue_dequeue(char **topic, char **payload);
void data_queue_stop(void);

#endif /* DATA_QUEUE_H */

