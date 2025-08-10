# Modbus Client BB

JSON-configured Modbus client that receives configuration over MQTT and publishes Modbus poll results back via MQTT using Paho MQTT Async. JSON parsing is done with cJSON. Modbus access is via libmodbus if present.

Build
- Prereqs: CMake, gcc, pthreads, and prebuilt static libraries in `thirdparty/`:
  - `thirdparty/paho_mqtt/lib/libpaho-mqtt3a.a` and headers in `thirdparty/paho_mqtt/inc`

Steps:
- run ./mk.sh  

Runtime
- On start, connects to `tcp://test.mosquitto.org:1883` as `modbus_client_BB`
- Subscribes to:
  - `modbus/test` (demo)
  - `modbus/config` for configuration

Config JSON example to publish on `modbus/config` (includes MQTT settings and TCP-only Modbus)

```
{
  "mqtt": { "host": "test.mosquitto.org", "port": 1883, "client_id": "modbus_client_BB" },
  "slave_id": 1,
  "poll_interval_ms": 1000,
  "publish_interval_ms": 0,
  "publish_topic": "modbus/tele",
  "tcp": { "host": "192.168.1.10", "port": 502 },
  "reads": [
    { "name": "hr0", "function": "holding_registers", "address": 0, "count": 4 },
    { "name": "ir0", "function": "input_registers", "address": 0, "count": 4 },
    { "name": "coils", "function": "coils", "address": 0, "count": 8 }
  ]
}
```

Published telemetry format per read plan:
`{"name":"hr0","ts":1690000000,"data":[12,34,56,78]}`


configuration send over mqtt using "forgeedge/config/serial" number as topic:
```
{
    "forge_edge_id": "FE-001",
    "data_mode": "processed",          // "processed" or "raw"
    "mqtt": {
      "enabled": true,
      "security_mode": "tls",          // "none" or "tls"
      "broker": "192.168.1.100",
      "port": 8883,
      "client_id": "FE-001-client",
      "username": "forgeedge",
      "password": "forgeedge",
      "tls_config": {
        "ca_cert": "/etc/forgeedge/ca.crt",
        "client_cert": "/etc/forgeedge/client.crt",
        "client_key": "/etc/forgeedge/client.key",
        "verify_peer": true             // true = validate broker certificate
      }
    },
    "io_devices": [
      {
        "io_device_id": "IO-01",
        "ip": "192.168.1.10",
        "port": 502,
        "poll_interval_ms": 1000,
        "parameters": [
          { "name": "die-temperature", "type": "holding", "address": 0, "count": 1 },
          { "name": "pressure", "type": "coil", "address": 10, "count": 1 }
        ]
      },
      {
        "io_device_id": "IO-02",
        "ip": "192.168.1.11",
        "port": 502,
        "poll_interval_ms": 2000,
        "parameters": [
          { "name": "pump-status", "type": "coil", "address": 20, "count": 1 }
        ]
      }
    ]
  }
```
