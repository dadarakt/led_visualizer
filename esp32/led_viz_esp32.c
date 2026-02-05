// LED Visualizer - ESP32 Runtime Implementation

#include "led_viz_esp32.h"
#include "led_strip.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char *TAG = "led_viz";

// Runtime state
static struct {
  led_strip_handle_t strips[4];
  int num_strips;
  int num_leds[4];
  int target_fps;

  const Program *current_program;
  const Palette16 *current_palette;

  volatile bool running;
  int64_t start_time_us;
} state;

// Pixel buffer (written by programs, sent to strips)
static uint8_t pixel_buffer[4][144][3]; // [strip][led][rgb]

// PixelFunc implementation - writes to buffer
static void esp32_pixel(int strip, int led, uint8_t *r, uint8_t *g,
                        uint8_t *b) {
  if (strip < 0 || strip >= state.num_strips)
    return;
  if (led < 0 || led >= state.num_leds[strip])
    return;

  if (r && g && b) {
    pixel_buffer[strip][led][0] = *r;
    pixel_buffer[strip][led][1] = *g;
    pixel_buffer[strip][led][2] = *b;
  }
  *r = pixel_buffer[strip][led][0];
  *g = pixel_buffer[strip][led][1];
  *b = pixel_buffer[strip][led][2];
}

// Send pixel buffer to actual LED strips
static void refresh_strips(void) {
  for (int s = 0; s < state.num_strips; s++) {
    for (int i = 0; i < state.num_leds[s]; i++) {
      led_strip_set_pixel(state.strips[s], i, pixel_buffer[s][i][0],
                          pixel_buffer[s][i][1], pixel_buffer[s][i][2]);
    }
    led_strip_refresh(state.strips[s]);
  }
}

int led_viz_init(const LedVizConfig *config) {
  memset(&state, 0, sizeof(state));
  memset(pixel_buffer, 0, sizeof(pixel_buffer));

  state.num_strips = config->num_strips;
  state.target_fps = config->target_fps > 0 ? config->target_fps : 60;

  // Initialize each strip using ESP-IDF's led_strip component
  for (int i = 0; i < config->num_strips; i++) {
    state.num_leds[i] = config->strips[i].num_leds;

    led_strip_config_t strip_config = {
        .strip_gpio_num = config->strips[i].gpio,
        .max_leds = config->strips[i].num_leds,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };

    esp_err_t err =
        led_strip_new_rmt_device(&strip_config, &rmt_config, &state.strips[i]);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to create LED strip %d: %s", i,
               esp_err_to_name(err));
      // Cleanup already created strips
      for (int j = 0; j < i; j++) {
        led_strip_del(state.strips[j]);
      }
      return -1;
    }

    // Clear the strip
    led_strip_clear(state.strips[i]);
  }

  state.start_time_us = esp_timer_get_time();
  ESP_LOGI(TAG, "Initialized %d strips at %d FPS", state.num_strips,
           state.target_fps);

  return 0;
}

void led_viz_set_program(int index) {
  // Assumes 'programs' and 'NUM_PROGRAMS' are defined by user code
  extern const Program programs[];
  extern const int NUM_PROGRAMS;

  if (index >= 0 && index < NUM_PROGRAMS) {
    if (state.current_program && state.current_program->cleanup) {
      state.current_program->cleanup();
    }
    state.current_program = &programs[index];
    if (state.current_program->init) {
      state.current_program->init();
    }
    ESP_LOGI(TAG, "Switched to program: %s", state.current_program->name);
  }
}

void led_viz_set_palette(const Palette16 *palette) {
  state.current_palette = palette;
}

void led_viz_run(void) {
  if (!state.current_program) {
    ESP_LOGE(TAG, "No program set");
    return;
  }
  if (!state.current_palette) {
    state.current_palette = &PALETTE_RAINBOW;
  }

  state.running = true;
  int64_t frame_time_us = 1000000 / state.target_fps;

  ESP_LOGI(TAG, "Starting animation loop");

  while (state.running) {
    int64_t frame_start = esp_timer_get_time();

    // Calculate time since start
    double time_ms = (frame_start - state.start_time_us) / 1000.0;

    // Get max LEDs across strips for the update call
    int max_leds = 0;
    for (int i = 0; i < state.num_strips; i++) {
      if (state.num_leds[i] > max_leds)
        max_leds = state.num_leds[i];
    }

    // Run the program
    state.current_program->update(state.num_strips, max_leds, time_ms,
                                  esp32_pixel, *state.current_palette);

    // Send to hardware
    refresh_strips();

    // Frame timing
    int64_t frame_end = esp_timer_get_time();
    int64_t elapsed = frame_end - frame_start;
    if (elapsed < frame_time_us) {
      vTaskDelay(pdMS_TO_TICKS((frame_time_us - elapsed) / 1000));
    }
  }

  ESP_LOGI(TAG, "Animation loop stopped");
}

void led_viz_stop(void) { state.running = false; }

void led_viz_deinit(void) {
  state.running = false;

  for (int i = 0; i < state.num_strips; i++) {
    if (state.strips[i]) {
      led_strip_clear(state.strips[i]);
      led_strip_del(state.strips[i]);
      state.strips[i] = NULL;
    }
  }

  ESP_LOGI(TAG, "Deinitialized");
}
