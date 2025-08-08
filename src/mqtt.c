#include "MQTTAsync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>   // for sleep()
#include "mqtt.h"

#define MQTT_ADDRESS     "tcp://test.mosquitto.org:1883"
#define MQTT_CLIENTID    "modbus_client_BB"
#define MQTT_TOPIC       "modbus/test"
#define MQTT_QOS         1
#define MQTT_TIMEOUT     10000L

static MQTTAsync client;
static volatile int client_ready = 0;  // flag to mark connection status

// Connection lost callback
void connlost(void *context, char *cause) {
    printf("Connection lost. Cause: %s\n", cause);
    client_ready = 0;
    // Try reconnecting
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = NULL;
    conn_opts.onFailure = NULL;
    conn_opts.context = client;

    int rc = MQTTAsync_connect(client, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start reconnect, return code %d\n", rc);
    } else {
        printf("Reconnecting...\n");
    }
}

// Message arrived callback
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    printf("Message arrived on topic: %s\n", topicName);
    printf("Payload: %.*s\n", message->payloadlen, (char *)message->payload);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

// Delivery complete callback
void onDeliveryComplete(void *context, MQTTAsync_token token) {
    printf("Delivery complete for token: %d\n", token);
}

// Connection success callback
void onConnectSuccess(void *context, MQTTAsync_successData *response) {
    printf("Successfully connected to MQTT broker.\n");
    client_ready = 1;

    // Subscribe to topic
    int rc = MQTTAsync_subscribe(client, MQTT_TOPIC, MQTT_QOS, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to subscribe, return code %d\n", rc);
    }
}

// Connection failure callback
void onConnectFailure(void *context, MQTTAsync_failureData *response) {
    printf("Connection to MQTT broker failed.\n");
    client_ready = 0;
}

void *mqtt_thread_func(void *arg) {
    int rc;

    printf("Thread started\n");

    rc = MQTTAsync_create(&client, MQTT_ADDRESS, MQTT_CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to create client: %d\n", rc);
        return NULL;
    }

    printf("MQTT client created\n");

    MQTTAsync_setCallbacks(client, NULL, connlost, msgarrvd, onDeliveryComplete);

    printf("Callbacks set\n");

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnectSuccess;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;

    rc = MQTTAsync_connect(client, &conn_opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect: %d\n", rc);
        return NULL;
    }

    printf("Connecting...\n");

    // Keep thread alive to handle async events
    while (1) {
        sleep(1);
    }

    return NULL;
}

void mqtt_start() {
    pthread_t mqtt_thread;
    printf("Creating MQTT thread\n");
    int err = pthread_create(&mqtt_thread, NULL, mqtt_thread_func, NULL);
    if (err) {
        printf("Failed to create MQTT thread: %d\n", err);
        return;
    }
    printf("Detaching MQTT thread\n");
    pthread_detach(mqtt_thread);
    printf("mqtt_start() completed\n");
}

void mqtt_publish(const char *topic, const char *payload) {
    if (!client_ready) {
        printf("MQTT client not ready yet. Skipping publish.\n");
        return;
    }

    if (client == NULL) {
        printf("MQTT client handle is NULL. Skipping publish.\n");
        return;
    }

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    pubmsg.payload = (void *)payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = MQTT_QOS;
    pubmsg.retained = 0;

    int rc;
    if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to publish message, return code: %d\n", rc);
    } else {
        printf("Message published to topic %s\n", topic);
    }
}

