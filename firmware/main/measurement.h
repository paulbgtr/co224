#pragma once
#include <stdint.h>

typedef struct {
    uint16_t co2;
    float t;
    float rh;
    int64_t ts;
} measurement_t;
