#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_IO_DEVICES	5u
#define MAX_PARAMETERS	32u
#define MAX_STR_LEN	128u

#define DEFAULT_CONFIG_PATH	"/etc/forgeedge/config.json"
#define SERIAL_FILE_PATH	"/etc/forgeedge/serial.txt"

struct parameter {
	char name[MAX_STR_LEN];
	char type[16];		/* \"coil\", \"holding\", \"input\" */
	int address;
	int count;
	double scale;
};

struct io_device {
	char io_device_id[MAX_STR_LEN];
	char ip[MAX_STR_LEN];
	int port;
	int unit_id;
	int poll_interval_ms;
	int parameter_count;
	struct parameter parameters[MAX_PARAMETERS];
};

struct tls_config {
	char ca_cert[MAX_STR_LEN];
	char client_cert[MAX_STR_LEN];
	char client_key[MAX_STR_LEN];
	int verify_peer;
};

struct mqtt_config {
	int enabled;
	char security_mode[16];	/* \"none\" or \"tls\" */
	char broker[MAX_STR_LEN];
	int port;
	char client_id[MAX_STR_LEN];
	char username[MAX_STR_LEN];
	char password[MAX_STR_LEN];
	struct tls_config tls;
};

struct config {
	char forge_edge_id[MAX_STR_LEN];
	char data_mode[16];	/* \"processed\" or \"raw\" */
	struct mqtt_config mqtt;
	int io_device_count;
	struct io_device io_devices[MAX_IO_DEVICES];
};

int load_config_from_file(const char *path, struct config *cfg);
int save_config_to_file(const char *path, const struct config *cfg);
int load_serial(char *buf, size_t len);

#endif /* CONFIG_H */

