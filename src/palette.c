#include "palette.h"

// ColorFromPalette - matches FastLED behavior
// The 16 palette entries are spread across 0-255 (each entry spans 16 indices)
CRGB ColorFromPalette(const CRGBPalette16 palette, uint8_t index,
                      uint8_t brightness, TBlendType blendType) {
  // Which palette entry (0-15) and fractional position within
  uint8_t entry = index >> 4;        // 0-15
  uint8_t fraction = (index & 0x0F); // 0-15

  CRGB color;

  if (blendType == NOBLEND || fraction == 0) {
    // No blending - just use the entry directly
    color = palette[entry];
  } else {
    // Linear interpolation between entry and entry+1
    uint8_t next_entry = (entry + 1) & 0x0F; // wrap at 16
    const CRGB *c1 = &palette[entry];
    const CRGB *c2 = &palette[next_entry];

    // fraction is 0-15, scale to 0-255 for lerp
    uint8_t blend = fraction << 4;
    uint8_t inv_blend = 255 - blend;

    color.r = (uint8_t)(((uint16_t)c1->r * inv_blend + (uint16_t)c2->r * blend) >> 8);
    color.g = (uint8_t)(((uint16_t)c1->g * inv_blend + (uint16_t)c2->g * blend) >> 8);
    color.b = (uint8_t)(((uint16_t)c1->b * inv_blend + (uint16_t)c2->b * blend) >> 8);
  }

  // Apply brightness
  if (brightness < 255) {
    color.r = (uint8_t)(((uint16_t)color.r * brightness) >> 8);
    color.g = (uint8_t)(((uint16_t)color.g * brightness) >> 8);
    color.b = (uint8_t)(((uint16_t)color.b * brightness) >> 8);
  }

  return color;
}

// Built-in palettes matching FastLED definitions

const CRGBPalette16 RainbowColors_p = {
    {255, 0, 0},     {255, 63, 0},    {255, 127, 0},   {255, 191, 0},
    {255, 255, 0},   {127, 255, 0},   {0, 255, 0},     {0, 255, 127},
    {0, 255, 255},   {0, 127, 255},   {0, 0, 255},     {127, 0, 255},
    {255, 0, 255},   {255, 0, 127},   {255, 0, 63},    {255, 0, 31},
};

const CRGBPalette16 HeatColors_p = {
    {0, 0, 0},       {51, 0, 0},      {102, 0, 0},     {153, 0, 0},
    {204, 0, 0},     {255, 0, 0},     {255, 51, 0},    {255, 102, 0},
    {255, 153, 0},   {255, 204, 0},   {255, 255, 0},   {255, 255, 51},
    {255, 255, 102}, {255, 255, 153}, {255, 255, 204}, {255, 255, 255},
};

const CRGBPalette16 OceanColors_p = {
    {0, 0, 51},    {0, 0, 102},   {0, 51, 153},  {0, 102, 153},
    {0, 153, 153}, {0, 153, 204}, {0, 153, 255}, {0, 204, 255},
    {51, 204, 255}, {102, 229, 255}, {153, 255, 255}, {204, 255, 255},
    {153, 255, 204}, {102, 255, 178}, {51, 255, 153}, {0, 255, 127},
};

const CRGBPalette16 ForestColors_p = {
    {0, 51, 0},    {0, 68, 0},    {0, 85, 0},    {0, 102, 0},
    {0, 119, 0},   {0, 136, 0},   {34, 153, 0},  {68, 170, 0},
    {102, 187, 0}, {136, 204, 34}, {170, 221, 68}, {187, 238, 102},
    {153, 221, 85}, {119, 204, 68}, {85, 187, 51}, {51, 170, 34},
};

const CRGBPalette16 LavaColors_p = {
    {0, 0, 0},     {51, 0, 0},    {102, 0, 0},   {153, 0, 0},
    {204, 0, 0},   {255, 0, 0},   {255, 51, 0},  {255, 102, 0},
    {255, 153, 0}, {255, 102, 0}, {255, 51, 0},  {255, 0, 0},
    {204, 0, 0},   {153, 0, 0},   {102, 0, 0},   {51, 0, 0},
};

const CRGBPalette16 CloudColors_p = {
    {0, 0, 255},     {0, 51, 255},    {0, 102, 255},   {51, 153, 255},
    {102, 178, 255}, {153, 204, 255}, {204, 229, 255}, {255, 255, 255},
    {204, 229, 255}, {153, 204, 255}, {102, 178, 255}, {51, 153, 255},
    {0, 102, 255},   {0, 51, 255},    {0, 0, 255},     {0, 0, 204},
};

const CRGBPalette16 PartyColors_p = {
    {85, 0, 171},  {132, 0, 123}, {181, 0, 75},  {229, 0, 27},
    {232, 23, 0},  {184, 71, 0},  {171, 119, 0}, {171, 171, 0},
    {171, 85, 0},  {221, 34, 0},  {242, 0, 14},  {194, 0, 62},
    {143, 0, 113}, {95, 0, 161},  {47, 0, 208},  {0, 7, 249},
};

// Palette registry for UI
const PaletteEntry palette_registry[] = {
    {"Rainbow", &RainbowColors_p},
    {"Heat", &HeatColors_p},
    {"Ocean", &OceanColors_p},
    {"Forest", &ForestColors_p},
    {"Lava", &LavaColors_p},
    {"Cloud", &CloudColors_p},
    {"Party", &PartyColors_p},
};

const int NUM_PALETTES = 7;
