#ifndef MQTT_H
#define MQTT_H

void mqtt_start(); // Call from main to start MQTT logic in a new thread
void mqtt_publish(const char *topic, const char *payload); // Publish utility

#endif /* MQTT_H */

