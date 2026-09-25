#include "user_code.h"
#include "bm_config.h"
#include "bm_os.h"
#include "bsp.h"
#include "configuration.h"
#include "io.h"
#include "pubsub.h"
#include "serial_bridge.h"
#include "uptime.h"
#include <string.h>

#define NUMBER_OF_COMMANDS_QUEUED 10
#define COMMAND_WAIT_KEY "cmdWaitMs"

static uint32_t uptime_wait_ms = 60000;
static bool released = false;

typedef struct {
  char *topic;
  uint16_t topic_len;
  void *data;
  uint16_t data_len;
  uint8_t type;
  uint8_t version;
} CameraPubInfo;

static void free_info(CameraPubInfo *info) {
  bm_free(info->topic);
  bm_free(info->data);
  bm_free(info);
}

static void timer_cb(void *arg) {
  BmTimer t = (BmTimer)arg;
  CameraPubInfo *info = (CameraPubInfo *)bm_timer_get_id(t);

  released = true;

  BmErr err = bm_pub_wl(info->topic, info->topic_len, info->data,
                        info->data_len, info->type, info->version);
  if (err != BmOK) {
    bm_debug("Could not publish to topic: %.*s", info->topic_len, info->topic);
  }

  free_info(info);
  bm_timer_delete(t, 0);
}

static void sub_cb(uint64_t, const char *topic, uint16_t topic_len,
                   const uint8_t *data, uint16_t data_len, uint8_t type,
                   uint8_t version) {
  uint64_t uptime_ms = uptimeGetMs();

  // If uptime is greater than the wait threshold do nothing
  if (released || uptime_ms >= uptime_wait_ms) {
    return;
  }

  uint64_t trigger_ms = uptime_wait_ms - uptime_ms;

  CameraPubInfo *info = (CameraPubInfo *)bm_malloc(sizeof(CameraPubInfo));

  *info = (CameraPubInfo){
      .topic = (char *)bm_malloc(topic_len),
      .topic_len = topic_len,
      .data = bm_malloc(data_len),
      .data_len = data_len,
      .type = type,
      .version = version,
  };

  memcpy(info->topic, topic, topic_len);
  memcpy(info->data, data, data_len);

  BmTimer timer =
      bm_timer_create("cmd_hold", trigger_ms, false, info, timer_cb);
  if (!timer || bm_timer_start(timer, 0) != BmOK) {
    bm_debug("Could not start timer...");
    free_info(info);
  }
}

void setup() {
  serial_bridge_init();
  get_config_uint(BM_CFG_PARTITION_SYSTEM, COMMAND_WAIT_KEY,
                  sizeof(COMMAND_WAIT_KEY), &uptime_wait_ms);
  bm_sub("bmcam/cmd", sub_cb);
  IOWrite(&BB_VBUS_EN, 0);
  // ensure Vbus stable before enable Vout with a 5ms delay.
  vTaskDelay(pdMS_TO_TICKS(5));
  // enable Vout, 12V by default.
  IOWrite(&BB_PL_BUCK_EN, 0);
}

void loop() { serial_bridge_handle(); }
