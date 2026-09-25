#pragma once
#include "esp_err.h"
#include <stdint.h>

void sensor_init(void);
esp_err_t sensor_read(uint16_t *co2, float *t, float *rh);
