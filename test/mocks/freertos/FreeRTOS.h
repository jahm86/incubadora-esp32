#pragma once

#include <stdint.h>

typedef void* QueueHandle_t;
typedef void* TaskHandle_t;
typedef uint32_t TickType_t;

#define portMAX_DELAY 0xFFFFFFFFUL
#define pdTRUE 1
#define pdFALSE 0
#define pdMS_TO_TICKS(ms) (ms)
