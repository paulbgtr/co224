#include "sensor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "scd4x.h"

#define SDA_PIN 5
#define SCL_PIN 6
#define MAX_FAILS 5

static const char *TAG = "co2";
static int fails = 0;
static i2c_dev_t dev = {0};

void sensor_init(void) {
  i2cdev_init();
  scd4x_init_desc(&dev, 0, SDA_PIN, SCL_PIN);

  scd4x_wake_up(&dev);
  scd4x_stop_periodic_measurement(&dev);
  scd4x_reinit(&dev);
  scd4x_start_periodic_measurement(&dev);
}

static void sensor_restart(void) {
  scd4x_wake_up(&dev);
  scd4x_stop_periodic_measurement(&dev);
  vTaskDelay(pdMS_TO_TICKS(500));
  scd4x_start_periodic_measurement(&dev);
}

esp_err_t sensor_read(uint16_t *co2, float *t, float *rh) {
  esp_err_t err = scd4x_read_measurement(&dev, co2, t, rh);

  if (err == ESP_OK && (*co2 == 0 || *co2 > 40000))
    err = ESP_ERR_INVALID_RESPONSE;

  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Error reading data: %s", esp_err_to_name(err));

    if (++fails >= MAX_FAILS) {
      ESP_LOGE(TAG, "Reinitializing the sensor");
      sensor_restart();
      fails = 0;
    }
    return err;
  }

  fails = 0;
  return ESP_OK;
}
