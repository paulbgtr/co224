#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "measurement.h"
#include "sdkconfig.h"
#include "sensor.h"
#include "wifi.h"

#define SDA 5
#define SCL 6
#define MAX_FAILS 5

static const char *TAG = "main";

void app_main(void) {
  if (wifi_start(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD) != ESP_OK) {
    ESP_LOGE(TAG, "Unable to start Wi-Fi");
    return;
  }

  if (!wifi_wait_connected(pdMS_TO_TICKS(15000)))
    ESP_LOGW(TAG, "Wi-Fi hasn't connected yet. Connecting...");

  QueueHandle_t data_q = xQueueCreate(100, sizeof(measurement_t));
  QueueHandle_t latest_q = xQueueCreate(1, sizeof(measurement_t));

  if (sensor_start(data_q, latest_q) != ESP_OK) {
    ESP_LOGE(TAG, "Sensor couldn't load");
    return;
  }
}
