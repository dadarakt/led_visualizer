#pragma once

#include <stdint.h>

// Pixel access function: if r/g/b are non-NULL, sets the pixel. Always writes
// current values back to r/g/b.
typedef void (*PixelFunc)(int strip, int led, uint8_t *r, uint8_t *g,
                          uint8_t *b);

// Program interface
typedef struct Program {
  const char *name;
  void (*update)(int num_strips, int num_leds, double time_ms, PixelFunc pixel);
  void (*init)(void);    // optional: called when program becomes active
  void (*cleanup)(void); // optional: called when switching away
} Program;

// Program registry
extern const Program programs[];
extern const int NUM_PROGRAMS;
