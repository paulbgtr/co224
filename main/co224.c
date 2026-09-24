#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2cdev.h"
#include "scd4x.h"
#include <stdio.h>

#define SDA 5
#define SCL 6

void app_main(void) {
  i2cdev_init();

  i2c_dev_t sensor = {0};
  scd4x_init_desc(&sensor, 0, SDA, SCL);

  scd4x_wake_up(&sensor);
  scd4x_stop_periodic_measurement(&sensor);
  scd4x_reinit(&sensor);
  scd4x_start_periodic_measurement(&sensor);

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(5000));

    uint16_t co2;
    float temperature, humidity;
    scd4x_read_measurement(&sensor, &co2, &temperature, &humidity);

    printf("CO2: %u ppm, T: %.1f C, RH: %.1f %%\n", co2, temperature, humidity);
  }
}
