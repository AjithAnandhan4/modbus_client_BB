
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "dataq.h"

struct queue_item {
	char *topic;
	char *payload;
};

static struct queue_item *queue_buf;
static size_t q_head;
static size_t q_tail;
static size_t q_cap;
static int q_running;
static pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t q_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t q_not_full = PTHREAD_COND_INITIALIZER;

int data_queue_init(size_t capacity)
{
	if (capacity == 0)
		return -1;

	queue_buf = calloc(capacity, sizeof(*queue_buf));
	if (!queue_buf)
		return -1;

	q_cap = capacity;
	q_head = 0;
	q_tail = 0;
	q_running = 1;
	return 0;
}

void data_queue_destroy(void)
{
	size_t i;

	if (!queue_buf)
		return;

	pthread_mutex_lock(&q_lock);
	q_running = 0;
	pthread_cond_broadcast(&q_not_empty);
	pthread_cond_broadcast(&q_not_full);
	pthread_mutex_unlock(&q_lock);

	/* free remaining items */
	for (i = 0; i < q_cap; i++) {
		free(queue_buf[i].topic);
		free(queue_buf[i].payload);
	}

	free(queue_buf);
	queue_buf = NULL;
}

int data_queue_enqueue(const char *topic, const char *payload)
{
	size_t next;

	if (!topic || !payload || !queue_buf)
		return -1;

	pthread_mutex_lock(&q_lock);

	/* wait until not full or stopped */
	next = (q_tail + 1) % q_cap;
	while (next == q_head && q_running)
		pthread_cond_wait(&q_not_full, &q_lock);

	if (!q_running) {
		pthread_mutex_unlock(&q_lock);
		return -1;
	}

	queue_buf[q_tail].topic = strdup(topic);
	queue_buf[q_tail].payload = strdup(payload);
	q_tail = next;

	pthread_cond_signal(&q_not_empty);
	pthread_mutex_unlock(&q_lock);
	return 0;
}

int data_queue_dequeue(char **topic, char **payload)
{
	if (!topic || !payload || !queue_buf)
		return -1;

	pthread_mutex_lock(&q_lock);
	while (q_head == q_tail && q_running)
		pthread_cond_wait(&q_not_empty, &q_lock);

	if (!q_running && q_head == q_tail) {
		pthread_mutex_unlock(&q_lock);
		return -1;
	}

	*topic = queue_buf[q_head].topic;
	*payload = queue_buf[q_head].payload;
	queue_buf[q_head].topic = NULL;
	queue_buf[q_head].payload = NULL;
	q_head = (q_head + 1) % q_cap;

	pthread_cond_signal(&q_not_full);
	pthread_mutex_unlock(&q_lock);
	return 0;
}

void data_queue_stop(void)
{
	pthread_mutex_lock(&q_lock);
	q_running = 0;
	pthread_cond_broadcast(&q_not_empty);
	pthread_cond_broadcast(&q_not_full);
	pthread_mutex_unlock(&q_lock);
}

