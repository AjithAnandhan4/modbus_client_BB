/*
 * company/product : SyncForge/ Forge-Edge
 *
 * file : config.c 
 *
 * author : Ajith Anandhan 
 * 
 * This file handles configuration parsing and saving it for reset proof
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "cJSON.h"

static double get_json_double(cJSON *obj, const char *name, double dflt)
{
	cJSON *item;

	item = cJSON_GetObjectItem(obj, name);
	if (!item || !cJSON_IsNumber(item))
		return dflt;

	return item->valuedouble;
}

int load_config_from_file(const char *path, struct config *cfg)
{
	FILE *fp;
	long size;
	char *buf;
	cJSON *root;
	cJSON *tmp;
	int i, j;

	if (!path)
		path = DEFAULT_CONFIG_PATH;

	fp = fopen(path, "r");
	if (!fp)
		return -errno;

	if (fseek(fp, 0, SEEK_END) < 0) {
		fclose(fp);
		return -errno;
	}

	size = ftell(fp);
	if (size < 0) {
		fclose(fp);
		return -errno;
	}

	if (fseek(fp, 0, SEEK_SET) < 0) {
		fclose(fp);
		return -errno;
	}

	buf = malloc(size + 1);
	if (!buf) {
		fclose(fp);
		return -ENOMEM;
	}

	if (fread(buf, 1, size, fp) != (size_t)size) {
		free(buf);
		fclose(fp);
		return -EIO;
	}
	buf[size] = '\0';
	fclose(fp);

	root = cJSON_Parse(buf);
	free(buf);
	if (!root)
		return -EINVAL;

	memset(cfg, 0, sizeof(*cfg));

	tmp = cJSON_GetObjectItem(root, "forge_edge_id");
	if (tmp && cJSON_IsString(tmp))
		strncpy(cfg->forge_edge_id, tmp->valuestring,
			sizeof(cfg->forge_edge_id) - 1);

	tmp = cJSON_GetObjectItem(root, "data_mode");
	if (tmp && cJSON_IsString(tmp))
		strncpy(cfg->data_mode, tmp->valuestring,
			sizeof(cfg->data_mode) - 1);

	/* mqtt block */
	tmp = cJSON_GetObjectItem(root, "mqtt");
	if (tmp && cJSON_IsObject(tmp)) {
		cJSON *it;

		it = cJSON_GetObjectItem(tmp, "enabled");
		cfg->mqtt.enabled = cJSON_IsTrue(it);

		it = cJSON_GetObjectItem(tmp, "security_mode");
		if (it && cJSON_IsString(it))
			strncpy(cfg->mqtt.security_mode, it->valuestring,
				sizeof(cfg->mqtt.security_mode) - 1);

		it = cJSON_GetObjectItem(tmp, "broker");
		if (it && cJSON_IsString(it))
			strncpy(cfg->mqtt.broker, it->valuestring,
				sizeof(cfg->mqtt.broker) - 1);

		it = cJSON_GetObjectItem(tmp, "port");
		if (it && cJSON_IsNumber(it))
			cfg->mqtt.port = it->valueint;

		it = cJSON_GetObjectItem(tmp, "client_id");
		if (it && cJSON_IsString(it))
			strncpy(cfg->mqtt.client_id, it->valuestring,
				sizeof(cfg->mqtt.client_id) - 1);

		it = cJSON_GetObjectItem(tmp, "username");
		if (it && cJSON_IsString(it))
			strncpy(cfg->mqtt.username, it->valuestring,
				sizeof(cfg->mqtt.username) - 1);

		it = cJSON_GetObjectItem(tmp, "password");
		if (it && cJSON_IsString(it))
			strncpy(cfg->mqtt.password, it->valuestring,
				sizeof(cfg->mqtt.password) - 1);

		it = cJSON_GetObjectItem(tmp, "tls_config");
		if (it && cJSON_IsObject(it)) {
			cJSON *t;
			t = cJSON_GetObjectItem(it, "ca_cert");
			if (t && cJSON_IsString(t))
				strncpy(cfg->mqtt.tls.ca_cert, t->valuestring,
					sizeof(cfg->mqtt.tls.ca_cert) - 1);
			t = cJSON_GetObjectItem(it, "client_cert");
			if (t && cJSON_IsString(t))
				strncpy(cfg->mqtt.tls.client_cert, t->valuestring,
					sizeof(cfg->mqtt.tls.client_cert) - 1);
			t = cJSON_GetObjectItem(it, "client_key");
			if (t && cJSON_IsString(t))
				strncpy(cfg->mqtt.tls.client_key, t->valuestring,
					sizeof(cfg->mqtt.tls.client_key) - 1);
			t = cJSON_GetObjectItem(it, "verify_peer");
			cfg->mqtt.tls.verify_peer = cJSON_IsTrue(t);
		}
	}

	/* io_devices array */
	tmp = cJSON_GetObjectItem(root, "io_devices");
	if (tmp && cJSON_IsArray(tmp)) {
		cJSON *dev;
		i = 0;
		cJSON_ArrayForEach(dev, tmp) {
			cJSON *p;
			if (i >= MAX_IO_DEVICES)
				break;

			p = cJSON_GetObjectItem(dev, "io_device_id");
			if (p && cJSON_IsString(p))
				strncpy(cfg->io_devices[i].io_device_id, p->valuestring,
					sizeof(cfg->io_devices[i].io_device_id) - 1);

			p = cJSON_GetObjectItem(dev, "ip");
			if (p && cJSON_IsString(p))
				strncpy(cfg->io_devices[i].ip, p->valuestring,
					sizeof(cfg->io_devices[i].ip) - 1);

			p = cJSON_GetObjectItem(dev, "port");
			if (p && cJSON_IsNumber(p))
				cfg->io_devices[i].port = p->valueint;
			else
				cfg->io_devices[i].port = 502;

			p = cJSON_GetObjectItem(dev, "poll_interval_ms");
			if (p && cJSON_IsNumber(p))
				cfg->io_devices[i].poll_interval_ms = p->valueint;
			else
				cfg->io_devices[i].poll_interval_ms = 1000;

			/* parameters array */
			p = cJSON_GetObjectItem(dev, "parameters");
			if (p && cJSON_IsArray(p)) {
				cJSON *par;
				j = 0;
				cJSON_ArrayForEach(par, p) {
					cJSON *pn;
					if (j >= MAX_PARAMETERS)
						break;

					pn = cJSON_GetObjectItem(par, "name");
					if (pn && cJSON_IsString(pn))
						strncpy(cfg->io_devices[i].parameters[j].name,
							pn->valuestring,
							sizeof(cfg->io_devices[i].parameters[j].name) - 1);

					pn = cJSON_GetObjectItem(par, "type");
					if (pn && cJSON_IsString(pn))
						strncpy(cfg->io_devices[i].parameters[j].type,
							pn->valuestring,
							sizeof(cfg->io_devices[i].parameters[j].type) - 1);

					pn = cJSON_GetObjectItem(par, "address");
					if (pn && cJSON_IsNumber(pn))
						cfg->io_devices[i].parameters[j].address =
							pn->valueint;

					pn = cJSON_GetObjectItem(par, "count");
					if (pn && cJSON_IsNumber(pn))
						cfg->io_devices[i].parameters[j].count =
							pn->valueint;
					else
						cfg->io_devices[i].parameters[j].count = 1;

					cfg->io_devices[i].parameters[j].scale =
						get_json_double(par, "scale", 1.0);

					j++;
				}
				cfg->io_devices[i].parameter_count = j;
			}
			i++;
		}
		cfg->io_device_count = i;
	}

	cJSON_Delete(root);
	return 0;
}

int save_config_to_file(const char *path, const struct config *cfg)
{
	cJSON *root, *mqtt, *tls, *devices, *dev, *params, *p;
	char *out;
	int i, j;
	FILE *fp;

	if (!path)
		path = DEFAULT_CONFIG_PATH;

	root = cJSON_CreateObject();
	if (!root)
		return -ENOMEM;

	cJSON_AddStringToObject(root, "forge_edge_id", cfg->forge_edge_id);
	cJSON_AddStringToObject(root, "data_mode", cfg->data_mode);

	mqtt = cJSON_CreateObject();
	cJSON_AddBoolToObject(mqtt, "enabled", cfg->mqtt.enabled);
	cJSON_AddStringToObject(mqtt, "security_mode", cfg->mqtt.security_mode);
	cJSON_AddStringToObject(mqtt, "broker", cfg->mqtt.broker);
	cJSON_AddNumberToObject(mqtt, "port", cfg->mqtt.port);
	cJSON_AddStringToObject(mqtt, "client_id", cfg->mqtt.client_id);
	cJSON_AddStringToObject(mqtt, "username", cfg->mqtt.username);
	cJSON_AddStringToObject(mqtt, "password", cfg->mqtt.password);

	tls = cJSON_CreateObject();
	cJSON_AddStringToObject(tls, "ca_cert", cfg->mqtt.tls.ca_cert);
	cJSON_AddStringToObject(tls, "client_cert", cfg->mqtt.tls.client_cert);
	cJSON_AddStringToObject(tls, "client_key", cfg->mqtt.tls.client_key);
	cJSON_AddBoolToObject(tls, "verify_peer", cfg->mqtt.tls.verify_peer);
	cJSON_AddItemToObject(mqtt, "tls_config", tls);

	cJSON_AddItemToObject(root, "mqtt", mqtt);

	devices = cJSON_CreateArray();
	for (i = 0; i < cfg->io_device_count; i++) {
		dev = cJSON_CreateObject();
		cJSON_AddStringToObject(dev, "io_device_id", cfg->io_devices[i].io_device_id);
		cJSON_AddStringToObject(dev, "ip", cfg->io_devices[i].ip);
		cJSON_AddNumberToObject(dev, "port", cfg->io_devices[i].port);
		cJSON_AddNumberToObject(dev, "poll_interval_ms",
			cfg->io_devices[i].poll_interval_ms);

		params = cJSON_CreateArray();
		for (j = 0; j < cfg->io_devices[i].parameter_count; j++) {
			p = cJSON_CreateObject();
			cJSON_AddStringToObject(p, "name",
				cfg->io_devices[i].parameters[j].name);
			cJSON_AddStringToObject(p, "type",
				cfg->io_devices[i].parameters[j].type);
			cJSON_AddNumberToObject(p, "address",
				cfg->io_devices[i].parameters[j].address);
			cJSON_AddNumberToObject(p, "count",
				cfg->io_devices[i].parameters[j].count);
			cJSON_AddNumberToObject(p, "scale",
				cfg->io_devices[i].parameters[j].scale);
			cJSON_AddItemToArray(params, p);
		}
		cJSON_AddItemToObject(dev, "parameters", params);
		cJSON_AddItemToArray(devices, dev);
	}
	cJSON_AddItemToObject(root, "io_devices", devices);

	out = cJSON_Print(root);
	if (!out) {
		cJSON_Delete(root);
		return -ENOMEM;
	}

	fp = fopen(path, "w");
	if (!fp) {
		free(out);
		cJSON_Delete(root);
		return -errno;
	}
	fputs(out, fp);
	fclose(fp);
	free(out);
	cJSON_Delete(root);
	return 0;
}

int load_serial(char *buf, size_t len)
{
	FILE *fp;

	if (!buf || len == 0)
		return -EINVAL;

	fp = fopen(SERIAL_FILE_PATH, "r");
	if (!fp)
		return -errno;

	if (!fgets(buf, len, fp)) {
		fclose(fp);
		return -EIO;
	}
	fclose(fp);

	buf[strcspn(buf, "\n")] = '\0';
	return 0;
}

