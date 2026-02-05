#include "palette.h"

RGB palette_sample(const Palette16 palette, uint8_t index, uint8_t brightness,
                   bool interpolate) {
  uint8_t entry = index >> 4;
  uint8_t fraction = (index & 0x0F);

  RGB color;

  if (!interpolate || fraction == 0) {
    color = palette[entry];
  } else {
    uint8_t next_entry = (entry + 1) & 0x0F;
    const RGB *c1 = &palette[entry];
    const RGB *c2 = &palette[next_entry];

    uint8_t blend = fraction << 4;
    uint8_t inv_blend = 255 - blend;

    color.r =
        (uint8_t)(((uint16_t)c1->r * inv_blend + (uint16_t)c2->r * blend) >> 8);
    color.g =
        (uint8_t)(((uint16_t)c1->g * inv_blend + (uint16_t)c2->g * blend) >> 8);
    color.b =
        (uint8_t)(((uint16_t)c1->b * inv_blend + (uint16_t)c2->b * blend) >> 8);
  }

  if (brightness < 255) {
    color.r = (uint8_t)(((uint16_t)color.r * brightness) >> 8);
    color.g = (uint8_t)(((uint16_t)color.g * brightness) >> 8);
    color.b = (uint8_t)(((uint16_t)color.b * brightness) >> 8);
  }

  return color;
}

// Built-in palettes

const Palette16 PALETTE_RAINBOW = {
    {255, 0, 0},   {255, 63, 0},  {255, 127, 0}, {255, 191, 0},
    {255, 255, 0}, {127, 255, 0}, {0, 255, 0},   {0, 255, 127},
    {0, 255, 255}, {0, 127, 255}, {0, 0, 255},   {127, 0, 255},
    {255, 0, 255}, {255, 0, 127}, {255, 0, 63},  {255, 0, 31},
};

const Palette16 PALETTE_HEAT = {
    {0, 0, 0},       {51, 0, 0},      {102, 0, 0},     {153, 0, 0},
    {204, 0, 0},     {255, 0, 0},     {255, 51, 0},    {255, 102, 0},
    {255, 153, 0},   {255, 204, 0},   {255, 255, 0},   {255, 255, 51},
    {255, 255, 102}, {255, 255, 153}, {255, 255, 204}, {255, 255, 255},
};

const Palette16 PALETTE_OCEAN = {
    {0, 0, 51},      {0, 0, 102},     {0, 51, 153},    {0, 102, 153},
    {0, 153, 153},   {0, 153, 204},   {0, 153, 255},   {0, 204, 255},
    {51, 204, 255},  {102, 229, 255}, {153, 255, 255}, {204, 255, 255},
    {153, 255, 204}, {102, 255, 178}, {51, 255, 153},  {0, 255, 127},
};

const Palette16 PALETTE_FOREST = {
    {0, 51, 0},     {0, 68, 0},     {0, 85, 0},     {0, 102, 0},
    {0, 119, 0},    {0, 136, 0},    {34, 153, 0},   {68, 170, 0},
    {102, 187, 0},  {136, 204, 34}, {170, 221, 68}, {187, 238, 102},
    {153, 221, 85}, {119, 204, 68}, {85, 187, 51},  {51, 170, 34},
};

const Palette16 PALETTE_LAVA = {
    {0, 0, 0},     {51, 0, 0},    {102, 0, 0},  {153, 0, 0},
    {204, 0, 0},   {255, 0, 0},   {255, 51, 0}, {255, 102, 0},
    {255, 153, 0}, {255, 102, 0}, {255, 51, 0}, {255, 0, 0},
    {204, 0, 0},   {153, 0, 0},   {102, 0, 0},  {51, 0, 0},
};

const Palette16 PALETTE_CLOUD = {
    {0, 0, 255},     {0, 51, 255},    {0, 102, 255},   {51, 153, 255},
    {102, 178, 255}, {153, 204, 255}, {204, 229, 255}, {255, 255, 255},
    {204, 229, 255}, {153, 204, 255}, {102, 178, 255}, {51, 153, 255},
    {0, 102, 255},   {0, 51, 255},    {0, 0, 255},     {0, 0, 204},
};

const Palette16 PALETTE_PARTY = {
    {85, 0, 171},  {132, 0, 123}, {181, 0, 75},  {229, 0, 27},
    {232, 23, 0},  {184, 71, 0},  {171, 119, 0}, {171, 171, 0},
    {171, 85, 0},  {221, 34, 0},  {242, 0, 14},  {194, 0, 62},
    {143, 0, 113}, {95, 0, 161},  {47, 0, 208},  {0, 7, 249},
};

// Palette registry for UI
const PaletteEntry palette_registry[] = {
    {"Rainbow", &PALETTE_RAINBOW},
    {"Heat", &PALETTE_HEAT},
    {"Ocean", &PALETTE_OCEAN},
    {"Forest", &PALETTE_FOREST},
    {"Lava", &PALETTE_LAVA},
    {"Cloud", &PALETTE_CLOUD},
    {"Party", &PALETTE_PARTY},
};

const int NUM_PALETTES = 7;
