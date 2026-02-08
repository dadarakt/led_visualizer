#pragma once

#include "palette.h"
#include <stdint.h>

// Strip definition - defines a single LED strip or matrix
typedef struct {
  int num_leds;       // Total LEDs (auto-calculated as width*height for matrices)
  float position;     // 1D position (-1.0 to 1.0), 0.0 = center
  float length_cm;    // Physical length/width in centimeters
  int matrix_width;   // 0 = strip, >0 = matrix columns
  int matrix_height;  // 0 = strip, >0 = matrix rows
} StripDef;

// Pixel access function: if r/g/b are non-NULL, sets the pixel. Always writes
// current values back to r/g/b.
typedef void (*PixelFunc)(int strip, int led, uint8_t *r, uint8_t *g,
                          uint8_t *b);

// Program interface
typedef struct Program {
  const char *name;
  void (*update)(double time_ms, PixelFunc pixel, const Palette16 palette);
  void (*init)(void);    // optional: called when program becomes active
  void (*cleanup)(void); // optional: called when switching away
} Program;

// Runtime accessors (for built-in programs)
int get_num_strips(void);
int get_strip_num_leds(int strip);
float get_strip_position(int strip);
float get_strip_length_cm(int strip);

// Matrix accessors
int get_matrix_width(int strip);
int get_matrix_height(int strip);
bool is_matrix(int strip);
int get_matrix_index(int strip, int x, int y);

// Strip setup registry
extern const StripDef strip_setup[];
extern const int NUM_STRIPS;

// Program registry
extern const Program programs[];
extern const int NUM_PROGRAMS;
