// FreeRTOS stub for linting only
#pragma once

#include "FreeRTOS.h"

static inline void vTaskDelay(TickType_t ticks) { (void)ticks; }
static inline void vTaskDelete(TaskHandle_t task) { (void)task; }

static inline BaseType_t xTaskCreate(void (*fn)(void *), const char *name,
                                     uint32_t stack, void *arg, int prio,
                                     TaskHandle_t *handle) {
  (void)fn;
  (void)name;
  (void)stack;
  (void)arg;
  (void)prio;
  (void)handle;
  return pdPASS;
}
