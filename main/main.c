#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"

#define SDA 5
#define SCL 6
#define MAX_FAILS 5

static const char *TAG = "main";

void app_main(void) {
  sensor_init();

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));

    uint16_t co2;
    float t, rh;

    if (sensor_read(&co2, &t, &rh) != ESP_OK) {
      ESP_LOGI(TAG, "Error");
      continue;
    }

    ESP_LOGI(TAG, "CO2: %u ppm, T: %.1f C, RH: %.1f %%", co2, t, rh);
  }
}
