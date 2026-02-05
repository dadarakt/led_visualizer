// LED Visualizer SDK
// Include this header in your programs.c file

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <math.h>

// ============================================================================
// RGB Color Types (FastLED-compatible)
// ============================================================================

typedef struct {
  uint8_t r, g, b;
} CRGB;

typedef CRGB CRGBPalette16[16];

typedef enum { NOBLEND = 0, LINEARBLEND = 1 } TBlendType;

// ColorFromPalette - matches FastLED signature
// index: 0-255, maps across the 16 palette entries with interpolation
// brightness: 0-255 scales the output
// blendType: LINEARBLEND interpolates, NOBLEND snaps to nearest
CRGB ColorFromPalette(const CRGBPalette16 palette, uint8_t index,
                      uint8_t brightness, TBlendType blendType);

#define CRGB_RGB(r, g, b) ((CRGB){(r), (g), (b)})

// ============================================================================
// Built-in Palettes (FastLED-compatible names)
// ============================================================================

extern const CRGBPalette16 RainbowColors_p;
extern const CRGBPalette16 HeatColors_p;
extern const CRGBPalette16 OceanColors_p;
extern const CRGBPalette16 ForestColors_p;
extern const CRGBPalette16 LavaColors_p;
extern const CRGBPalette16 CloudColors_p;
extern const CRGBPalette16 PartyColors_p;

// ============================================================================
// Program Interface
// ============================================================================

// Pixel access function: sets the pixel at (strip, led) to the given RGB color
typedef void (*PixelFunc)(int strip, int led, uint8_t *r, uint8_t *g,
                          uint8_t *b);

// Program definition
typedef struct Program {
  const char *name;
  void (*update)(int num_strips, int num_leds, double time_ms, PixelFunc pixel,
                 const CRGBPalette16 palette);
  void (*init)(void);    // optional: called when program becomes active
  void (*cleanup)(void); // optional: called when switching away
} Program;

// ============================================================================
// User must define these in their programs.c:
// ============================================================================
//
//   const Program programs[] = {
//       {"My Program", my_update_func, NULL, NULL},
//       // ... more programs
//   };
//
//   const int NUM_PROGRAMS = sizeof(programs) / sizeof(programs[0]);
//
// ============================================================================
