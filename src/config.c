#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "cJSON.h"
#include "config.h"

static ModbusFunctionType parse_function_string(const char *func) {
  if (!func) return MODBUS_FUNC_HOLDING_REGISTERS;
  if (strcasecmp(func, "coils") == 0) return MODBUS_FUNC_COILS;
  if (strcasecmp(func, "discrete_inputs") == 0 || strcasecmp(func, "discrete") == 0) return MODBUS_FUNC_DISCRETE_INPUTS;
  if (strcasecmp(func, "holding_registers") == 0 || strcasecmp(func, "holding") == 0) return MODBUS_FUNC_HOLDING_REGISTERS;
  if (strcasecmp(func, "input_registers") == 0 || strcasecmp(func, "input") == 0) return MODBUS_FUNC_INPUT_REGISTERS;
  return MODBUS_FUNC_HOLDING_REGISTERS;
}

bool config_update_from_json(const char *json_text, ModbusConfig *out_config) {
  if (!json_text || !out_config) return false;

  cJSON *root = cJSON_Parse(json_text);
  if (!root) return false;

  memset(out_config, 0, sizeof(ModbusConfig));

  // defaults (TCP-only)
  out_config->poll_interval_ms = 1000;
  out_config->publish_interval_ms = 0; // 0 = publish on every poll cycle
  out_config->tcp.port = 502;

  // common
  const cJSON *slave = cJSON_GetObjectItemCaseSensitive(root, "slave_id");
  if (cJSON_IsNumber(slave)) out_config->slave_id = slave->valueint;

  const cJSON *poll = cJSON_GetObjectItemCaseSensitive(root, "poll_interval_ms");
  if (cJSON_IsNumber(poll)) out_config->poll_interval_ms = (uint32_t)poll->valuedouble;

  const cJSON *pubint = cJSON_GetObjectItemCaseSensitive(root, "publish_interval_ms");
  if (cJSON_IsNumber(pubint)) out_config->publish_interval_ms = (uint32_t)pubint->valuedouble;

  const cJSON *pub = cJSON_GetObjectItemCaseSensitive(root, "publish_topic");
  if (cJSON_IsString(pub) && pub->valuestring) snprintf(out_config->publish_topic, sizeof(out_config->publish_topic), "%s", pub->valuestring);

  // tcp
  const cJSON *tcp = cJSON_GetObjectItemCaseSensitive(root, "tcp");
  if (cJSON_IsObject(tcp)) {
    const cJSON *host = cJSON_GetObjectItemCaseSensitive(tcp, "host");
    const cJSON *port = cJSON_GetObjectItemCaseSensitive(tcp, "port");
    if (cJSON_IsString(host) && host->valuestring)
      snprintf(out_config->tcp.host, sizeof(out_config->tcp.host), "%s", host->valuestring);
    if (cJSON_IsNumber(port)) out_config->tcp.port = port->valueint;
  }

  // reads
  const cJSON *reads = cJSON_GetObjectItemCaseSensitive(root, "reads");
  if (cJSON_IsArray(reads)) {
    int idx = 0;
    cJSON *it = NULL;
    cJSON_ArrayForEach(it, reads) {
      if (!cJSON_IsObject(it) || idx >= 16) break;
      const cJSON *name = cJSON_GetObjectItemCaseSensitive(it, "name");
      const cJSON *func = cJSON_GetObjectItemCaseSensitive(it, "function");
      const cJSON *addr = cJSON_GetObjectItemCaseSensitive(it, "address");
      const cJSON *count = cJSON_GetObjectItemCaseSensitive(it, "count");
      ModbusReadPlan *plan = &out_config->reads[idx];
      plan->function = parse_function_string(cJSON_IsString(func) ? func->valuestring : NULL);
      plan->address = cJSON_IsNumber(addr) ? addr->valueint : 0;
      plan->count = cJSON_IsNumber(count) ? count->valueint : 1;
      if (cJSON_IsString(name) && name->valuestring)
        snprintf(plan->name, sizeof(plan->name), "%s", name->valuestring);
      else
        snprintf(plan->name, sizeof(plan->name), "read_%d", idx);
      idx++;
    }
    out_config->num_reads = idx;
  }

  cJSON_Delete(root);
  return true;
}

bool config_mqtt_settings_from_json(const char *json_text, MqttClientSettings *out) {
  if (!json_text || !out) return false;
  cJSON *root = cJSON_Parse(json_text);
  if (!root) return false;
  memset(out, 0, sizeof(*out));
  const cJSON *mqtt = cJSON_GetObjectItemCaseSensitive(root, "mqtt");
  if (cJSON_IsObject(mqtt)) {
    const cJSON *host = cJSON_GetObjectItemCaseSensitive(mqtt, "host");
    const cJSON *port = cJSON_GetObjectItemCaseSensitive(mqtt, "port");
    const cJSON *cid = cJSON_GetObjectItemCaseSensitive(mqtt, "client_id");
    if (cJSON_IsString(host) && host->valuestring) snprintf(out->host, sizeof(out->host), "%s", host->valuestring);
    if (cJSON_IsNumber(port)) out->port = port->valueint;
    if (cJSON_IsString(cid) && cid->valuestring) snprintf(out->client_id, sizeof(out->client_id), "%s", cid->valuestring);
  }
  cJSON_Delete(root);
  return true;
}


