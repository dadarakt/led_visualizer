// LED Visualizer - ESP32 Runtime
// Thin layer that runs programs on real hardware using ESP-IDF led_strip

#pragma once

#include "led_viz.h"
#include <stdbool.h>

// Configuration for a single LED strip
typedef struct {
  int gpio;
  int num_leds;
} StripConfig;

// Runtime configuration
typedef struct {
  StripConfig strips[4];
  int num_strips;
  int target_fps;
} LedVizConfig;

// Initialize the runtime with given configuration
// Returns 0 on success, -1 on error
int led_viz_init(const LedVizConfig *config);

// Set the active program (index into your programs[] array)
void led_viz_set_program(int index);

// Set the active palette
void led_viz_set_palette(const Palette16 *palette);

// Run the animation loop (blocking - call from a FreeRTOS task)
// This will call program->update() at target_fps and refresh the strips
void led_viz_run(void);

// Stop the animation loop
void led_viz_stop(void);

// Cleanup and release resources
void led_viz_deinit(void);
