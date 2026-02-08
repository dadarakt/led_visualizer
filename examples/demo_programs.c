// Example LED programs for led_viz
// Usage: led_viz ./demo_programs.c

#include <led_viz.h>

// Strip setup - 4 strips + 1 matrix
const StripDef strip_setup[] = {
    {.num_leds = 144, .position = -0.7f, .length_cm = 100.0f},
    {.num_leds = 144, .position = -0.35f, .length_cm = 100.0f},
    {.num_leds = 256, .position = 0.0f, .length_cm = 32.0f,
     .matrix_width = 32, .matrix_height = 8},
    {.num_leds = 144, .position = 0.35f, .length_cm = 100.0f},
    {.num_leds = 144, .position = 0.7f, .length_cm = 100.0f},
};
const int NUM_STRIPS = sizeof(strip_setup) / sizeof(strip_setup[0]);

// Simple rainbow scroll
static void rainbow_update(double time_ms, PixelFunc pixel,
                           const Palette16 palette) {
  long shift_value = time_ms / 20;

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);
    for (int i = 0; i < num_leds; i++) {
      uint8_t index = (shift_value + i) % 255;
      RGB color = palette_sample(palette, index, 255, true);
      pixel(s, i, &color.r, &color.g, &color.b);
    }
  }
}

// Breathing effect
static void breathe_update(double time_ms, PixelFunc pixel,
                           const Palette16 palette) {
  float t = (float)(time_ms / 1000.0);
  float breath = (sinf(t * 2.0f) + 1.0f) * 0.5f; // 0.0 - 1.0
  uint8_t brightness = (uint8_t)(breath * 255.0f);

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);
    for (int i = 0; i < num_leds; i++) {
      uint8_t index = (uint8_t)((float)i / (float)num_leds * 255.0f);
      RGB color = palette_sample(palette, index, brightness, true);
      pixel(s, i, &color.r, &color.g, &color.b);
    }
  }
}

// Sparkle effect
static void sparkle_update(double time_ms, PixelFunc pixel,
                           const Palette16 palette) {
  (void)palette;

  // Simple pseudo-random based on time
  unsigned int seed = (unsigned int)(time_ms * 10.0);

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);
    for (int i = 0; i < num_leds; i++) {
      // Darken all pixels first
      uint8_t r = 0, g = 0, b = 0;
      pixel(s, i, &r, &g, &b);
    }
  }

  // Add some random sparkles
  for (int k = 0; k < 20; k++) {
    seed = seed * 1103515245 + 12345;
    int s = (seed >> 16) % num_strips;
    int num_leds = get_strip_num_leds(s);
    seed = seed * 1103515245 + 12345;
    int i = (seed >> 16) % num_leds;
    uint8_t r = 255, g = 255, b = 255;
    pixel(s, i, &r, &g, &b);
  }
}

// Required exports
const Program programs[] = {
    {"Rainbow", rainbow_update, NULL, NULL},
    {"Breathe", breathe_update, NULL, NULL},
    {"Sparkle", sparkle_update, NULL, NULL},
};

const int NUM_PROGRAMS = sizeof(programs) / sizeof(programs[0]);
