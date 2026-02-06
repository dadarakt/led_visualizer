// FreeRTOS stub for linting only
#pragma once

#include <stdint.h>

typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef int BaseType_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS pdTRUE
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
