#pragma once
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

esp_err_t wifi_start(void);
bool wifi_wait_connected(TickType_t timeout);
