// Example ESP32 main.c
// Shows how to use led_viz runtime with your programs

#include "led_viz_esp32.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Include your programs file (same file used in desktop visualizer!)
// This file must define:
//   - strip_setup[]  : Array of StripDef with num_leds and position per strip
//   - NUM_STRIPS     : Number of strips
//   - programs[]     : Array of Program definitions
//   - NUM_PROGRAMS   : Number of programs
//
// Example programs.c strip setup (strips and matrices):
//
//   const StripDef strip_setup[] = {
//       // LED strip: 144 LEDs, 1m
//       {.num_leds = 144, .position = -0.5f, .length_cm = 100.0f},
//       // LED matrix: 16x16, 16cm wide (use get_matrix_index() to address)
//       {.num_leds = 256, .position = 0.5f, .length_cm = 16.0f,
//        .matrix_width = 16, .matrix_height = 16},
//   };
//   const int NUM_STRIPS = 2;
//
#include "programs.c"

static const char *TAG = "main";

void app_main(void) {
  ESP_LOGI(TAG, "LED Visualizer starting");

  // Configure GPIO pins for each strip (strip config comes from programs.c)
  LedVizConfig config = {
      .gpio_pins = {18, 19, 21, 22}, // GPIO for strip 0, 1, 2, 3
      .target_fps = 60,
  };

  if (led_viz_init(&config) != 0) {
    ESP_LOGE(TAG, "Failed to initialize LED strips");
    return;
  }

  // Set initial program and palette
  led_viz_set_program(0);
  led_viz_set_palette(&PALETTE_RAINBOW);

  // Run animation (this blocks)
  // In a real app, you'd run this in a task and handle input separately
  led_viz_run();

  // Cleanup (only reached if led_viz_stop() is called)
  led_viz_deinit();
}

// Alternative: Run in a FreeRTOS task so you can handle input
/*
static void led_task(void *arg) {
    led_viz_run();
    vTaskDelete(NULL);
}

void app_main(void) {
    LedVizConfig config = {
        .gpio_pins = {18, 19, 21, 22},
        .target_fps = 60,
    };

    led_viz_init(&config);
    led_viz_set_program(0);
    led_viz_set_palette(&PALETTE_RAINBOW);

    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);

    // Main loop can now handle buttons, WiFi, etc.
    int current_program = 0;
    while (1) {
        if (button_pressed()) {
            current_program = (current_program + 1) % NUM_PROGRAMS;
            led_viz_set_program(current_program);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
*/
