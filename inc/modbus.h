#ifndef MODBUS_H
#define MODBUS_H

#include <stdbool.h>
#include "config.h"

// Initialize and start worker thread based on config; publishes results via provided publish callback
typedef void (*publish_fn_t)(const char *topic, const char *payload);

bool modbus_worker_start(const ModbusConfig *config, publish_fn_t publish_fn);
void modbus_worker_stop(void);
bool modbus_worker_is_running(void);

#endif // MODBUS_H


