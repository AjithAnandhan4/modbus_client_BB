#ifndef MODBUS_IF_H
#define MODBUS_IF_H

#include "config.h"

int start_modbus_process(const struct config *cfg);
void stop_modbus_process(void);

#endif /* MODBUS_IF_H */

