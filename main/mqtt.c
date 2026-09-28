// mqtt.c
#include "mqtt.h"
#include "esp_check.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "measurement.h"
#include "mqtt_client.h"
#include <stdio.h>

#define CONNECTED_BIT BIT0

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t client;
static EventGroupHandle_t events;
static QueueHandle_t data_q;

static char device_id[16];
static char topic_data[48];
static char topic_status[48];

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
  switch ((esp_mqtt_event_id_t)id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected to the broker");
    esp_mqtt_client_publish(client, topic_status, "online", 0, 1, 1);
    xEventGroupSetBits(events, CONNECTED_BIT);
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "Disconnected from the broker");
    xEventGroupClearBits(events, CONNECTED_BIT);
    break;

  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT Error");
    break;

  default:
    break;
  }
}

static void mqtt_task(void *arg) {
  measurement_t m;
  char payload[64];

  while (1) {
    xEventGroupWaitBits(events, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    if (xQueueReceive(data_q, &m, pdMS_TO_TICKS(1000)) != pdTRUE)
      continue;

    snprintf(payload, sizeof payload, "{\"co2\":%u,\"t\":%.1f,\"rh\":%.1f}",
             m.co2, m.t, m.rh);

    if (esp_mqtt_client_publish(client, topic_data, payload, 0, 1, 1) < 0)
      ESP_LOGW(TAG, "Couldn't send the request, the measurement was lost");
  }
}

esp_err_t mqtt_start(QueueHandle_t data_queue) {
  data_q = data_queue;
  events = xEventGroupCreate();

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(device_id, sizeof device_id, "co2-%02x%02x%02x", mac[3], mac[4],
           mac[5]);
  snprintf(topic_data, sizeof topic_data, "co2/%s/data", device_id);
  snprintf(topic_status, sizeof topic_status, "co2/%s/status", device_id);
  ESP_LOGI(TAG, "Device ID: %s", device_id);

  esp_mqtt_client_config_t cfg = {
      .broker =
          {
              .address.uri = CONFIG_MQTT_URI,
              .verification.crt_bundle_attach = esp_crt_bundle_attach,
          },
      .credentials =
          {
              .username = CONFIG_MQTT_USERNAME,
              .authentication.password = CONFIG_MQTT_PASSWORD,
              .client_id = device_id,
          },
      .session.last_will =
          {
              .topic = topic_status,
              .msg = "offline",
              .qos = 1,
              .retain = 1,
          },
  };

  client = esp_mqtt_client_init(&cfg);
  if (client == NULL)
    return ESP_FAIL;

  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, on_event, NULL);
  ESP_RETURN_ON_ERROR(esp_mqtt_client_start(client), TAG, "start");

  if (xTaskCreate(mqtt_task, "mqtt", 4096, NULL, 4, NULL) != pdPASS)
    return ESP_ERR_NO_MEM;

  return ESP_OK;
}
