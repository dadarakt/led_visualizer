// Example ESP32 main.c
// Shows how to use led_viz runtime with your programs

#include "led_viz_esp32.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Include your programs (same file used in visualizer!)
#include "programs.c"

static const char *TAG = "main";

void app_main(void) {
  ESP_LOGI(TAG, "LED Visualizer starting");

  // Configure strips
  LedVizConfig config = {
      .strips =
          {
              {.gpio = 18, .num_leds = 144},
              {.gpio = 19, .num_leds = 144},
              {.gpio = 21, .num_leds = 144},
              {.gpio = 22, .num_leds = 144},
          },
      .num_strips = 4,
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
    // ... init code ...

    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);

    // Main loop can now handle buttons, WiFi, etc.
    while (1) {
        if (button_pressed()) {
            led_viz_set_program((current_program + 1) % NUM_PROGRAMS);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
*/
