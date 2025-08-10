#ifndef MQTT_H
#define MQTT_H

#include "config.h"

int mqtt_start(const struct config *cfg);
void mqtt_stop(void);
int mqtt_publish(const char *topic, const char *payload);

#endif /* MQTT_H */

