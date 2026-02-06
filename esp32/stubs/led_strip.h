// ESP-IDF led_strip component stub for linting only
#pragma once

#include "esp_err.h"
#include <stdint.h>

typedef void *led_strip_handle_t;

typedef enum {
  LED_PIXEL_FORMAT_GRB,
  LED_PIXEL_FORMAT_RGB,
} led_pixel_format_t;

typedef enum {
  LED_MODEL_WS2812,
  LED_MODEL_SK6812,
} led_model_t;

typedef struct {
  int strip_gpio_num;
  uint32_t max_leds;
  led_pixel_format_t led_pixel_format;
  led_model_t led_model;
} led_strip_config_t;

typedef struct {
  uint32_t resolution_hz;
} led_strip_rmt_config_t;

static inline esp_err_t led_strip_new_rmt_device(const led_strip_config_t *cfg,
                                                 const led_strip_rmt_config_t *rmt,
                                                 led_strip_handle_t *handle) {
  (void)cfg;
  (void)rmt;
  (void)handle;
  return ESP_OK;
}

static inline esp_err_t led_strip_set_pixel(led_strip_handle_t h, uint32_t idx,
                                            uint8_t r, uint8_t g, uint8_t b) {
  (void)h;
  (void)idx;
  (void)r;
  (void)g;
  (void)b;
  return ESP_OK;
}

static inline esp_err_t led_strip_refresh(led_strip_handle_t h) {
  (void)h;
  return ESP_OK;
}

static inline esp_err_t led_strip_clear(led_strip_handle_t h) {
  (void)h;
  return ESP_OK;
}

static inline esp_err_t led_strip_del(led_strip_handle_t h) {
  (void)h;
  return ESP_OK;
}
