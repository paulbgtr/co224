#pragma once
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_err.h"

esp_err_t wifi_start(const char *ssid, const char *password);
bool wifi_wait_connected(TickType_t timeout);
