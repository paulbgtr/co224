#include "sensor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "i2cdev.h"
#include "measurement.h"
#include "scd4x.h"

#define SDA_PIN 5
#define SCL_PIN 6
#define PERIOD_MS 5000
#define MAX_FAILS 5

static const char *TAG = "sensor";

static i2c_dev_t dev;
static QueueHandle_t data_q;
static QueueHandle_t latest_q;

static void sensor_restart(void) {
  scd4x_wake_up(&dev);
  scd4x_stop_periodic_measurement(&dev);
  scd4x_reinit(&dev);
  scd4x_start_periodic_measurement(&dev);
}

static bool is_valid(const measurement_t *m) {
  return m->co2 > 0 && m->co2 <= 40000 && m->t > -10 && m->t < 60;
}

static void sensor_task(void *arg) {
  int fails = 0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));

    measurement_t m;
    esp_err_t err = scd4x_read_measurement(&dev, &m.co2, &m.t, &m.rh);

    if (err != ESP_OK || !is_valid(&m)) {
      ESP_LOGW(TAG, "Bad reading (&d times)", ++fails);

      if (fails >= MAX_FAILS) {
        ESP_LOGE(TAG, "Reinitializing the sensor");
        sensor_restart();
        fails = 0;
      }
      continue;
    }
    fails = 0;

    ESP_LOGI(TAG, "CO2: %u ppm, T: %.1f C, RH: %.1f %%", m.co2, m.t, m.rh);

    if (data_q && xQueueSend(data_q, &m, 0) != pdTRUE)
      ESP_LOGI(TAG, "Queue is full, the measurement is lost");

    if (latest_q)
      xQueueOverwrite(latest_q, &m);
  }
}

esp_err_t sensor_start(QueueHandle_t data_queue, QueueHandle_t latest_queue) {
  data_q = data_queue;
  latest_q = latest_queue;

  esp_err_t err = i2cdev_init();
  if (err != ESP_OK)
    return err;

  err = scd4x_init_desc(&dev, 0, SDA_PIN, SCL_PIN);
  if (err != ESP_OK)
    return err;

  sensor_restart();

  if (xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL) != pdPASS)
    return ESP_ERR_NO_MEM;

  return ESP_OK;
}
