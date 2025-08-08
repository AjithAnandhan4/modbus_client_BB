#include <stdio.h>
#include <unistd.h>
#include "json.h"
#include "mqtt.h"

int main() {
    const char *json_string = "{\"device\":\"modbus_client\",\"status\":\"active\",\"count\":10}";
    parse_and_print_json(json_string);

    printf("starting modbus client...\n");

    mqtt_start();
    sleep(5);
    mqtt_publish("modbus/test", "Hello MQTT from BB...!!!");
#if 1
    while(1) {
//	    mqtt_publish("modbus/tele", "{\"status\": \"active\"}");
     	    sleep(2);
    }
#endif
	    mqtt_publish("modbus/tele", "{\"status\": \"active\"}");
    return 0;
}

