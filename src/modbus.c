#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "modbus.h"

#ifdef HAVE_LIBMODBUS
#include <modbus.h>
#endif

typedef struct WorkerState {
  pthread_t thread;
  int should_run;
  ModbusConfig config;
  publish_fn_t publisher;
} WorkerState;

static WorkerState g_state;

static void publish_json(publish_fn_t pub, const char *topic, const char *json) {
  if (pub && topic && json) pub(topic, json);
}

static char *build_payload_json(const ModbusConfig *cfg,
                                const char *name,
                                const uint16_t *u16, int u16_count,
                                const uint8_t *u8, int u8_count) {
  char *buf = NULL;
  size_t cap = 1024 + (size_t)(u16_count + u8_count) * 12;
  buf = (char*)malloc(cap);
  if (!buf) return NULL;
  long ts = time(NULL);
  size_t off = 0;
  off += snprintf(buf + off, cap - off, "{\"name\":\"%s\",\"ts\":%ld,\"data\":[", name, ts);
  int first = 1;
  for (int i = 0; i < u16_count; ++i) {
    off += snprintf(buf + off, cap - off, "%s%u", first ? "" : ",", (unsigned)u16[i]);
    first = 0;
  }
  for (int i = 0; i < u8_count; ++i) {
    off += snprintf(buf + off, cap - off, "%s%u", first ? "" : ",", (unsigned)u8[i]);
    first = 0;
  }
  snprintf(buf + off, cap - off, "]}");
  return buf;
}

static void *worker_thread(void *arg) {
  (void)arg;
#ifdef HAVE_LIBMODBUS
  modbus_t *ctx = NULL;
  ctx = modbus_new_tcp(g_state.config.tcp.host, g_state.config.tcp.port);
  if (!ctx) {
    fprintf(stderr, "modbus: failed to create context\n");
    g_state.should_run = 0;
  }

  if (ctx) {
    modbus_set_slave(ctx, g_state.config.slave_id);
    if (modbus_connect(ctx) == -1) {
      fprintf(stderr, "modbus: connect failed\n");
      modbus_free(ctx);
      ctx = NULL;
      g_state.should_run = 0;
    }
  }
#endif

  while (g_state.should_run) {
#ifdef HAVE_LIBMODBUS
    if (!ctx) break;
    for (int i = 0; i < g_state.config.num_reads; ++i) {
      ModbusReadPlan *rp = &g_state.config.reads[i];
      int addr = rp->address;
      int cnt = rp->count;
      int rc = -1;
      uint16_t regbuf[256];
      uint8_t bitbuf[512];
      if (cnt > 256) cnt = 256;
      if (rp->function == MODBUS_FUNC_HOLDING_REGISTERS) {
        rc = modbus_read_registers(ctx, addr, cnt, regbuf);
        if (rc >= 0) {
          char *json = build_payload_json(&g_state.config, rp->name, regbuf, rc, NULL, 0);
          if (json) {
            publish_json(g_state.publisher, g_state.config.publish_topic, json);
            free(json);
          }
        }
      } else if (rp->function == MODBUS_FUNC_INPUT_REGISTERS) {
        rc = modbus_read_input_registers(ctx, addr, cnt, regbuf);
        if (rc >= 0) {
          char *json = build_payload_json(&g_state.config, rp->name, regbuf, rc, NULL, 0);
          if (json) { publish_json(g_state.publisher, g_state.config.publish_topic, json); free(json);}        
        }
      } else if (rp->function == MODBUS_FUNC_COILS) {
        if (cnt > 512) cnt = 512;
        rc = modbus_read_bits(ctx, addr, cnt, bitbuf);
        if (rc >= 0) {
          char *json = build_payload_json(&g_state.config, rp->name, NULL, 0, bitbuf, rc);
          if (json) { publish_json(g_state.publisher, g_state.config.publish_topic, json); free(json);}        
        }
      } else if (rp->function == MODBUS_FUNC_DISCRETE_INPUTS) {
        if (cnt > 512) cnt = 512;
        rc = modbus_read_input_bits(ctx, addr, cnt, bitbuf);
        if (rc >= 0) {
          char *json = build_payload_json(&g_state.config, rp->name, NULL, 0, bitbuf, rc);
          if (json) { publish_json(g_state.publisher, g_state.config.publish_topic, json); free(json);}        
        }
      }
    }
#endif
    usleep(g_state.config.poll_interval_ms * 1000);
  }

#ifdef HAVE_LIBMODBUS
  if (ctx) {
    modbus_close(ctx);
    modbus_free(ctx);
  }
#endif
  return NULL;
}

bool modbus_worker_start(const ModbusConfig *config, publish_fn_t publish_fn) {
  if (!config || !publish_fn) return false;
  if (g_state.should_run) return true;
  memset(&g_state, 0, sizeof(g_state));
  g_state.config = *config;
  g_state.publisher = publish_fn;
  g_state.should_run = 1;
  if (pthread_create(&g_state.thread, NULL, worker_thread, NULL) != 0) {
    g_state.should_run = 0;
    return false;
  }
  pthread_detach(g_state.thread);
  return true;
}

void modbus_worker_stop(void) {
  if (!g_state.should_run) return;
  g_state.should_run = 0;
}

bool modbus_worker_is_running(void) { return g_state.should_run != 0; }


