#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2cdev.h"
#include "scd4x.h"
#include <stdio.h>

#define SDA 5
#define SCL 6
#define MAX_FAILS 5

static const char *TAG = "co2";

void app_main(void) {
  i2cdev_init();

  i2c_dev_t sensor = {0};
  scd4x_init_desc(&sensor, 0, SDA, SCL);

  scd4x_wake_up(&sensor);
  scd4x_stop_periodic_measurement(&sensor);
  scd4x_reinit(&sensor);
  scd4x_start_periodic_measurement(&sensor);

  int fails = 0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));

    uint16_t co2;
    float t, rh;
    esp_err_t err = scd4x_read_measurement(&sensor, &co2, &t, &rh);

    if (err != ESP_OK) {
      fails++;
      ESP_LOGW(TAG, "Error reading sensor (%d times): %s", fails,
               esp_err_to_name(err));

      if (fails >= MAX_FAILS) {
        ESP_LOGE(TAG, "Sensor not working, reinitializing...");
        scd4x_wake_up(&sensor);
        scd4x_stop_periodic_measurement(&sensor);
        vTaskDelay(pdMS_TO_TICKS(500));
        scd4x_start_periodic_measurement(&sensor);
        fails = 0;
      }
      continue;
    }

    if (co2 == 0 || co2 > 40000 || t < -10 || t > 60) {
      ESP_LOGW(TAG, "Useless data: CO2=%u, T=%.1f, skipping...", co2, t);
      continue;
    }

    fails = 0;
    ESP_LOGI(TAG, "CO2: %u ppm, T: %.1f C, RH: %.1f %%", co2, t, rh);
  }
}
