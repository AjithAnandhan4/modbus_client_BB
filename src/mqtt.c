/*
 * mqtt.c 
 *
 * Notes:
 *  - This code expects Paho Async API (MQTTAsync_*) headers available.
 *  - TLS is supported via MQTTAsync_SSLOptions when security_mode == "tls".
 *  - Paho has internal persistence/queueing if configured; here we enqueue
 *    strings in userspace and call MQTTAsync_sendMessage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "mqtt.h"
#include "dataq.h"
#include "config.h"

#include "MQTTAsync.h" /* paho async header; adjust include path as needed */

#define Q_CAPACITY 1024
#define CLIENT_KEEPALIVE 60

static const struct config *global_cfg;
static pthread_t mqtt_thread;
static volatile int mqtt_running;
static volatile int connected = 0;
static MQTTAsync client;

/* forward */
static void *mqtt_thread_fn(void *arg);

/* simple callback functions */

static void connlost(void *context, char *cause)
{
	(void)context;
	fprintf(stderr, "[MQTT] connection lost: %s\n", cause ? cause : "unknown");
	connected = 0;
	/* Paho will call connectionLost; the client must reconnect by calling
	 * MQTTAsync_connect again in a real production implementation.
	 * For simplicity rely on the mqtt thread to attempt reconnects.
	 */
}

static void on_send(void *context, MQTTAsync_successData *response)
{
	(void)context;
	(void)response;
	/* delivery acknowledged */
	return 1;
}

static void on_connect_success(void *context, MQTTAsync_successData *response)
{
	(void)context;
	(void)response;
	fprintf(stderr, "[MQTT] connected (on_connect_success)\n");
}

static void on_connect_failure(void *context, MQTTAsync_failureData *response)
{
	(void)context;
	(void)response;
	fprintf(stderr, "[MQTT] connect failed\n");
}

static int message_arrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    // Just free the message and return 1 to indicate success
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

/*
 * build_connect_options - prepare MQTTAsync_connectOptions with TLS if needed.
 */
static int build_connect_options(MQTTAsync_connectOptions *conn_opts,
				 MQTTAsync_SSLOptions *ssl_opts,
				 const struct mqtt_config *mcfg)
{
	memset(conn_opts, 0, sizeof(*conn_opts));
	conn_opts->keepAliveInterval = CLIENT_KEEPALIVE;
	conn_opts->cleansession = 1;
	conn_opts->onSuccess = on_connect_success;
	conn_opts->onFailure = on_connect_failure;
	conn_opts->context = NULL;

	if (mcfg->enabled && strcmp(mcfg->security_mode, "tls") == 0) {
		memset(ssl_opts, 0, sizeof(*ssl_opts));
		ssl_opts->trustStore = mcfg->tls.ca_cert[0] ? mcfg->tls.ca_cert : NULL;
		ssl_opts->keyStore = mcfg->tls.client_cert[0] ? mcfg->tls.client_cert : NULL;
		ssl_opts->privateKey = mcfg->tls.client_key[0] ? mcfg->tls.client_key : NULL;
		ssl_opts->enableServerCertAuth = mcfg->tls.verify_peer ? 1 : 0;
		conn_opts->ssl = ssl_opts;
	} else {
		conn_opts->ssl = NULL;
	}

	return 0;
}

static int mqtt_publish_msg(const char *topic, const char *payload)
{
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	pubmsg.payload = (void *)payload;
	pubmsg.payloadlen = (int)strlen(payload);
	pubmsg.qos = 1;
	pubmsg.retained = 0;

	opts.onSuccess = on_send;
	opts.onFailure = NULL;
	opts.context = NULL;

	rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
	if (rc != MQTTASYNC_SUCCESS) {
		fprintf(stderr, "[MQTT] sendMessage failed: %d\n", rc);
		return -1;
	}
	return 0;
}

static void *mqtt_thread_fn(void *arg)
{
	const struct config *cfg = (const struct config *)arg;
	char address[256];
	int rc;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	char *topic = NULL;
	char *payload = NULL;

	if (strcmp(cfg->mqtt.security_mode, "tls") == 0)
		snprintf(address, sizeof(address), "ssl://%s:%d", cfg->mqtt.broker, cfg->mqtt.port);
	else
		snprintf(address, sizeof(address), "tcp://%s:%d", cfg->mqtt.broker, cfg->mqtt.port);

	printf("uri:%s\n", address);

	rc = MQTTAsync_create(&client, address, cfg->mqtt.client_id,
			      MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTASYNC_SUCCESS) {
		fprintf(stderr, "[MQTT] MQTTAsync_create failed: %d\n", rc);
		mqtt_running = 0;
		return NULL;
	}
	fprintf(stderr, "Connecting to broker '%s' on port %d with client ID '%s'\n",
        cfg->mqtt.broker, cfg->mqtt.port, cfg->mqtt.client_id);

	MQTTAsync_setCallbacks(client, NULL, connlost, message_arrived, NULL);

	conn_opts.keepAliveInterval = 60;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = on_connect_success;
	conn_opts.onFailure = on_connect_failure;
	conn_opts.context = client;

	/* attempt connect with retries */
	while (mqtt_running) {
		if (!connected) {
			rc = MQTTAsync_connect(client, &conn_opts);
			if (rc == MQTTASYNC_SUCCESS) {
				connected = 1;
				fprintf(stderr, "[MQTT] connect in progress\n");
				/* wait briefly for onSuccess callback or assume connected */
				sleep(1);
			} else {
				fprintf(stderr, "[MQTT] connect failed rc=%d, retrying in 2s\n", rc);
				sleep(2);
				continue;
			}
		}

		/* publish any queued messages */
		if (data_queue_dequeue(&topic, &payload) == 0) {
			if (topic && payload) {
				mqtt_publish_msg(topic, payload);
				free(topic);
				free(payload);
			}
		} else {
			/* queue stopped or interrupted; wait briefly */
			sleep(1);
		}
	}

	/* disconnect cleanly */
	if (connected) {
		MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
		MQTTAsync_disconnect(client, &disc_opts);
	}

	MQTTAsync_destroy(&client);
	return NULL;
}

int mqtt_start(const struct config *cfg)
{
	int rc;

	if (!cfg)
		return -1;

	/* init queue */
	rc = data_queue_init(Q_CAPACITY);
	if (rc)
		return -1;

	global_cfg = cfg;
	mqtt_running = 1;

	rc = pthread_create(&mqtt_thread, NULL, mqtt_thread_fn, (void *)cfg);
	if (rc) {
		mqtt_running = 0;
		data_queue_destroy();
		return -1;
	}
	return 0;
}

void mqtt_stop(void)
{
	mqtt_running = 0;
	data_queue_stop();
	pthread_join(mqtt_thread, NULL);
	data_queue_destroy();
}

int mqtt_publish(const char *topic, const char *payload)
{
	return data_queue_enqueue(topic, payload);
}

