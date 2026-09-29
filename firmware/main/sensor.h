#pragma once
#include "esp_err.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"

esp_err_t sensor_start(QueueHandle_t data_queue, QueueHandle_t latest_queue);
