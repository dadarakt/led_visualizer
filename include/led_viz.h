// LED Visualizer SDK
// Include this header in your programs.c file

#pragma once

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// RGB Color Type
// ============================================================================

typedef struct {
  uint8_t r, g, b;
} RGB;

// ============================================================================
// Color Palettes
// ============================================================================

// 16-entry gradient palette
typedef RGB Palette16[16];

// Sample a color from a palette
// - index: 0-255, maps across the 16 palette entries
// - brightness: 0-255, scales the output
// - interpolate: true for smooth gradients, false for nearest-entry snap
RGB palette_sample(const Palette16 palette, uint8_t index, uint8_t brightness,
                   bool interpolate);

// Built-in palettes
extern const Palette16 PALETTE_RAINBOW;
extern const Palette16 PALETTE_HEAT;
extern const Palette16 PALETTE_OCEAN;
extern const Palette16 PALETTE_FOREST;
extern const Palette16 PALETTE_LAVA;
extern const Palette16 PALETTE_CLOUD;
extern const Palette16 PALETTE_PARTY;

// ============================================================================
// Strip Configuration
// ============================================================================

// Strip definition - defines a single LED strip
typedef struct {
  int num_leds;
  float position;  // 1D position (-1.0 to 1.0), 0.0 = center
} StripDef;

// Runtime accessors (call from update functions)
int get_num_strips(void);
int get_strip_num_leds(int strip);
float get_strip_position(int strip);

// Internal: called by runtime to set strip setup (do not call from programs)
void _led_viz_set_strip_setup(const StripDef *setup, int num_strips);

// ============================================================================
// Program Interface
// ============================================================================

// Pixel setter function: sets the pixel at (strip, led) to the given RGB color
typedef void (*PixelFunc)(int strip, int led, uint8_t *r, uint8_t *g,
                          uint8_t *b);

// Program definition
typedef struct Program {
  const char *name;
  void (*update)(double time_ms, PixelFunc pixel, const Palette16 palette);
  void (*init)(void);    // optional: called when program becomes active
  void (*cleanup)(void); // optional: called when switching away
} Program;

// ============================================================================
// Your programs.c must define:
//
//   const StripDef strip_setup[] = {
//       {.num_leds = 144, .position = -0.5f},
//       {.num_leds = 144, .position = 0.5f},
//   };
//   const int NUM_STRIPS = sizeof(strip_setup) / sizeof(strip_setup[0]);
//
//   const Program programs[] = {
//       {"My Program", my_update_func, NULL, NULL},
//   };
//   const int NUM_PROGRAMS = sizeof(programs) / sizeof(programs[0]);
//
// ============================================================================
