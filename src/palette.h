#pragma once

#include <stdbool.h>
#include <stdint.h>

// RGB color type
typedef struct {
  uint8_t r, g, b;
} RGB;

// 16-entry gradient palette
typedef RGB Palette16[16];

// Sample a color from a palette
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

// Palette registry for UI
typedef struct {
  const char *name;
  const Palette16 *palette;
} PaletteEntry;

extern const PaletteEntry palette_registry[];
extern const int NUM_PALETTES;
