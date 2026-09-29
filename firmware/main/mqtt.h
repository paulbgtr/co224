#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

esp_err_t mqtt_start(QueueHandle_t data_queue);
