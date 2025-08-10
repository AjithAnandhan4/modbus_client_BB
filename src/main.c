/*
 * company/ product:syncForge/ Forge-Edge
 *
 * file: main.c 
 * 
 * author: Ajith Anandhan 
 *
 * This file demonstrates the startup sequence:
 *  - load serial
 *  - attempt to load saved config
 *  - start mqtt thread
 *  - start modbus process threads if config present
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "config.h"
#include "mqtt.h"
#include "modbus.h"

static volatile int running = 1;
static struct config cfg;

static void sigint_handler(int sig)
{
	(void)sig;
	running = 0;
}

int main(int argc, char **argv)
{
	char serial[MAX_STR_LEN];
	int rc;

	(void)argc;
	(void)argv;

	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);

	rc = load_serial(serial, sizeof(serial));
	if (rc) {
		fprintf(stderr, "failed to load serial (%d)\n", rc);
		return EXIT_FAILURE;
	}

	memset(&cfg, 0, sizeof(cfg));
	strncpy(cfg.forge_edge_id, serial, sizeof(cfg.forge_edge_id) - 1);
	strncpy(cfg.data_mode, "processed", sizeof(cfg.data_mode) - 1);

	/* try load saved config; if missing, we wait for mqtt-provided config */
	if (load_config_from_file(DEFAULT_CONFIG_PATH, &cfg) == 0)
		printf("loaded config from %s\n", DEFAULT_CONFIG_PATH);
	else
		printf("no saved config; will wait for remote config\n");

	rc = mqtt_start(&cfg);
	if (rc) {
		fprintf(stderr, "mqtt_start failed (%d)\n", rc);
	}

	if (cfg.io_device_count > 0) {
		rc = start_modbus_process(&cfg);
		if (rc)
			fprintf(stderr, "start_modbus_workers failed (%d)\n", rc);
	}

	while (running)
		sleep(1);

	stop_modbus_process();
	mqtt_stop();

	return 0;
}

