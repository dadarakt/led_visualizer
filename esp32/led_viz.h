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
// Program Interface
// ============================================================================

// Pixel setter function: sets the pixel at (strip, led) to the given RGB color
typedef void (*PixelFunc)(int strip, int led, uint8_t *r, uint8_t *g,
                          uint8_t *b);

// Program definition
typedef struct Program {
  const char *name;
  void (*update)(int num_strips, int num_leds, double time_ms, PixelFunc pixel,
                 const Palette16 palette);
  void (*init)(void);    // optional: called when program becomes active
  void (*cleanup)(void); // optional: called when switching away
} Program;

// ============================================================================
// Your programs.c must define:
//
//   const Program programs[] = {
//       {"My Program", my_update_func, NULL, NULL},
//   };
//   const int NUM_PROGRAMS = sizeof(programs) / sizeof(programs[0]);
//
// ============================================================================
