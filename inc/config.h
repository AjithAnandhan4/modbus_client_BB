#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>

// Modbus function types supported
typedef enum ModbusFunctionType {
  MODBUS_FUNC_COILS = 1,              // modbus_read_bits
  MODBUS_FUNC_DISCRETE_INPUTS = 2,    // modbus_read_input_bits
  MODBUS_FUNC_HOLDING_REGISTERS = 3,  // modbus_read_registers
  MODBUS_FUNC_INPUT_REGISTERS = 4     // modbus_read_input_registers
} ModbusFunctionType;

typedef struct ModbusReadPlan {
  ModbusFunctionType function;
  int address;
  int count;
  char name[64];
} ModbusReadPlan;

typedef struct ModbusTcpConfig {
  char host[128];
  int port;
} ModbusTcpConfig;

typedef struct ModbusConfig {
  int slave_id;
  uint32_t poll_interval_ms;
  uint32_t publish_interval_ms; // independent telemetry publish interval (optional)
  char publish_topic[128];
  // Reads
  ModbusReadPlan reads[16];
  int num_reads;

  // Connection-specific
  ModbusTcpConfig tcp;
} ModbusConfig;

// Global configuration state
bool config_update_from_json(const char *json_text, ModbusConfig *out_config);

// MQTT client parameters from JSON config
typedef struct MqttClientSettings {
  char host[128];
  int port;
  char client_id[128];
} MqttClientSettings;

bool config_mqtt_settings_from_json(const char *json_text, MqttClientSettings *out);

#endif // CONFIG_H


