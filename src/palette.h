#pragma once

#include <stdint.h>

// RGB color type - matches FastLED's CRGB
typedef struct {
  uint8_t r, g, b;
} CRGB;

// 16-entry gradient palette - matches FastLED's CRGBPalette16
typedef CRGB CRGBPalette16[16];

// Blend types - matches FastLED
typedef enum { NOBLEND = 0, LINEARBLEND = 1 } TBlendType;

// ColorFromPalette - matches FastLED signature
// index: 0-255, maps across the 16 palette entries with interpolation
// brightness: 0-255 scales the output
// blendType: LINEARBLEND interpolates, NOBLEND snaps to nearest
CRGB ColorFromPalette(const CRGBPalette16 palette, uint8_t index,
                      uint8_t brightness, TBlendType blendType);

// Convenience macro for defining palette entries (matches FastLED)
#define CRGB_RGB(r, g, b) ((CRGB){(r), (g), (b)})

// Built-in palettes - match FastLED names
extern const CRGBPalette16 RainbowColors_p;
extern const CRGBPalette16 HeatColors_p;
extern const CRGBPalette16 OceanColors_p;
extern const CRGBPalette16 ForestColors_p;
extern const CRGBPalette16 LavaColors_p;
extern const CRGBPalette16 CloudColors_p;
extern const CRGBPalette16 PartyColors_p;

// Palette registry for simulator UI
typedef struct {
  const char *name;
  const CRGBPalette16 *palette;
} PaletteEntry;

extern const PaletteEntry palette_registry[];
extern const int NUM_PALETTES;
