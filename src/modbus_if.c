/*
 * modbus_if.c - per-IO-device modbus data aquisition implementation using libmodbus.
 *
 *  - creates a modbus_tcp context to the device ip:port
 *  - polls parameters per device->poll_interval_ms
 *  - creates JSON payload and enqueues to mqtt_publish()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <modbus.h>
#include "modbus.h"
#include "config.h"
#include "cJSON.h"
#include "mqtt.h"

#define MAX_WORKERS 64

static pthread_t worker_threads[MAX_WORKERS];
static int worker_active;
static const struct config *global_cfg;

static void build_and_enqueue_reading(const struct io_device *dev)
{
	cJSON *root, *data_arr;
	char *s;
	char topic[256];
	time_t now = time(NULL);

	root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "edge_id", global_cfg->forge_edge_id);
	cJSON_AddStringToObject(root, "io_device_id", dev->io_device_id);
	cJSON_AddNumberToObject(root, "timestamp", (double)now);

	data_arr = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "data", data_arr);

	for (int i = 0; i < dev->parameter_count; i++) {
		const struct parameter *p = &dev->parameters[i];
		cJSON *entry = cJSON_CreateObject();
		if (!entry)
			continue;
		cJSON_AddStringToObject(entry, "name", p->name);
		cJSON_AddStringToObject(entry, "type", p->type);

		if (strcmp(p->type, "coil") == 0) {
			uint8_t bits[128];
			int rc = modbus_read_bits(NULL, p->address, p->count, bits);
			/* NOTE: above is placeholder. We must use a modbus context. */
			/* In the real code below we use ctx for per-device connection. */
			(void)rc;
		}

		cJSON_AddItemToArray(data_arr, entry);
	}

	s = cJSON_PrintUnformatted(root);
	if (s) {
		snprintf(topic, sizeof(topic), "forgeedge/%s/%s/data",
			 global_cfg->forge_edge_id, dev->io_device_id);
		mqtt_publish(topic, s);
		free(s);
	}
	cJSON_Delete(root);
}

/*
 * device_thread - worker per device
 */
static void *device_thread(void *arg)
{
	const struct io_device *dev = (const struct io_device *)arg;
	modbus_t *ctx = NULL;
	int rc;

	ctx = modbus_new_tcp(dev->ip, dev->port);
	if (!ctx) {
		fprintf(stderr, "[MODBUS] failed create ctx %s\n", dev->io_device_id);
		return NULL;
	}

	if (dev->unit_id > 0)
		modbus_set_slave(ctx, dev->unit_id);

	if (modbus_connect(ctx) == -1) {
		fprintf(stderr, "[MODBUS] connect failed %s:%d\n", dev->ip, dev->port);
		modbus_free(ctx);
		return NULL;
	}

	while (worker_active) {
		cJSON *root = cJSON_CreateObject();
		cJSON *arr = cJSON_CreateArray();
		char topic[256];
		char *out;
		time_t now = time(NULL);

		cJSON_AddStringToObject(root, "edge_id", global_cfg->forge_edge_id);
		cJSON_AddStringToObject(root, "io_device_id", dev->io_device_id);
		cJSON_AddNumberToObject(root, "timestamp", (double)now);
		cJSON_AddItemToObject(root, "data", arr);

		for (int i = 0; i < dev->parameter_count; i++) {
			const struct parameter *p = &dev->parameters[i];
			cJSON *entry = cJSON_CreateObject();
			cJSON_AddStringToObject(entry, "name", p->name);
			cJSON_AddStringToObject(entry, "type", p->type);

			if (strcmp(p->type, "coil") == 0) {
				uint8_t *bits;
				bits = calloc(p->count, sizeof(uint8_t));
				if (!bits)
					continue;
				rc = modbus_read_bits(ctx, p->address, p->count, bits);
				if (rc >= 0) {
					if (p->count == 1)
						cJSON_AddNumberToObject(entry, "raw", bits[0]);
					else {
						cJSON *a = cJSON_CreateArray();
						for (int k = 0; k < p->count; k++)
							cJSON_AddItemToArray(a,
								cJSON_CreateNumber(bits[k]));
						cJSON_AddItemToObject(entry, "raw", a);
					}
				}
				free(bits);
			} else if (strcmp(p->type, "holding") == 0 ||
				   strcmp(p->type, "input") == 0) {
				uint16_t *regs;
				regs = calloc(p->count, sizeof(uint16_t));
				if (!regs)
					continue;
				if (strcmp(p->type, "holding") == 0)
					rc = modbus_read_registers(ctx, p->address,
								  p->count, regs);
				else
					rc = modbus_read_input_registers(ctx, p->address,
									 p->count, regs);
				if (rc >= 0) {
					if (p->count == 1) {
						double v = regs[0] * p->scale;
						if (strcmp(global_cfg->data_mode, "raw") == 0)
							cJSON_AddNumberToObject(entry, "raw", regs[0]);
						else
							cJSON_AddNumberToObject(entry, "value", v);
					} else {
						cJSON *a = cJSON_CreateArray();
						for (int k = 0; k < p->count; k++)
							cJSON_AddItemToArray(a,
								cJSON_CreateNumber(regs[k] * p->scale));
						cJSON_AddItemToObject(entry, "value", a);
					}
				}
				free(regs);
			} else {
				/* unknown type: skip */
			}

			cJSON_AddItemToArray(arr, entry);
		}

		out = cJSON_PrintUnformatted(root);
		if (out) {
			snprintf(topic, sizeof(topic), "forgeedge/%s/%s/data",
				 global_cfg->forge_edge_id, dev->io_device_id);
			mqtt_publish(topic, out);
			free(out);
		}
		cJSON_Delete(root);

		/* sleep by poll interval, use nanosleep for better accuracy */
		struct timespec ts;
		ts.tv_sec = dev->poll_interval_ms / 1000;
		ts.tv_nsec = (dev->poll_interval_ms % 1000) * 1000000;
		nanosleep(&ts, NULL);
	}

	modbus_close(ctx);
	modbus_free(ctx);
	return NULL;
}

int start_modbus_process(const struct config *cfg)
{
	int i;
	int rc;

	if (!cfg)
		return -1;

	global_cfg = cfg;
	worker_active = 1;

	for (i = 0; i < cfg->io_device_count && i < MAX_WORKERS; i++) {
		rc = pthread_create(&worker_threads[i], NULL,
				    device_thread, (void *)&cfg->io_devices[i]);
		if (rc) {
			fprintf(stderr, "[MODBUS] failed create thread %d\n", i);
		}
	}
	return 0;
}

void stop_modbus_process(void)
{
	int i;

	worker_active = 0;
	for (i = 0; i < MAX_WORKERS; i++) {
		pthread_join(worker_threads[i], NULL);
	}
}

