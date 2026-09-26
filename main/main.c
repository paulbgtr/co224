#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "measurement.h"
#include "sensor.h"

#define SDA 5
#define SCL 6
#define MAX_FAILS 5

static const char *TAG = "main";

void app_main(void) {
  QueueHandle_t data_q = xQueueCreate(100, sizeof(measurement_t));
  QueueHandle_t latest_q = xQueueCreate(1, sizeof(measurement_t));

  if (sensor_start(data_q, latest_q) != ESP_OK) {
    ESP_LOGE(TAG, "Sensor couldn't load");
    return;
  }
}
