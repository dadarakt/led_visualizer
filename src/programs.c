#include "programs.h"
#include <math.h>
#include <stddef.h>

// Strip setup - 4 strips evenly spaced
const StripDef strip_setup[] = {
    {.num_leds = 144, .position = -0.5f, .length_cm = 100.0f},
    {.num_leds = 144, .position = -0.17f, .length_cm = 100.0f},
    {.num_leds = 144, .position = 0.17f, .length_cm = 100.0f},
    {.num_leds = 144, .position = 0.5f, .length_cm = 100.0f},
};
const int NUM_STRIPS = sizeof(strip_setup) / sizeof(strip_setup[0]);

static void heartbeat_update(double time_ms, PixelFunc pixel,
                             const Palette16 palette) {
  double t = time_ms / 1000.0;

  double beat_period = 60.0 / 72.0; // ~72 bpm
  double beat_phase = fmod(t, beat_period) / beat_period;
  double lub = exp(-pow((beat_phase - 0.15) * 12.0, 2.0));
  double dub = exp(-pow((beat_phase - 0.35) * 12.0, 2.0));
  float heartbeat = 0.6f + 0.4f * fmax(lub, dub);

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);
    for (int i = 0; i < num_leds; i++) {
      uint8_t index = (uint8_t)((float)i / (float)num_leds * 255.0f +
                                t * 50.0f); // scroll over time
      uint8_t brightness = (uint8_t)(255.0f * heartbeat);
      RGB color = palette_sample(palette, index, brightness, true);

      pixel(s, i, &color.r, &color.g, &color.b);
    }
  }
}

static void rainbow_update(double time_ms, PixelFunc pixel,
                           const Palette16 palette) {
  float t = (float)(time_ms / 1000.0);

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);
    for (int i = 0; i < num_leds; i++) {
      uint8_t index =
          (uint8_t)((float)i / (float)num_leds * 255.0f + t * 60.0f);
      RGB color = palette_sample(palette, index, 255, true);
      pixel(s, i, &color.r, &color.g, &color.b);
    }
  }
}

static void solid_white_update(double time_ms, PixelFunc pixel,
                               const Palette16 palette) {
  (void)time_ms;
  (void)palette;

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);
    for (int i = 0; i < num_leds; i++) {
      uint8_t r = 255, g = 255, b = 255;
      pixel(s, i, &r, &g, &b);
    }
  }
}

static void comet_update(double time_ms, PixelFunc pixel,
                         const Palette16 palette) {
  float t = (float)(time_ms / 1000.0);
  float tail_length = 25.0f;

  int num_strips = get_num_strips();
  for (int s = 0; s < num_strips; s++) {
    int num_leds = get_strip_num_leds(s);

    // Pseudo-random per-strip values based on strip index
    unsigned int seed = (unsigned int)(s * 2654435761u);
    float speed = 0.3f + (float)(seed % 100) / 200.0f;   // 0.3 - 0.8
    float phase_offset = (float)(seed % 1000) / 1000.0f; // 0.0 - 1.0

    // Position of comet head moving top to bottom (high index to low)
    float cycle_pos = fmodf(t * speed + phase_offset, 1.0f);
    float head_pos = (1.0f - cycle_pos) * (float)num_leds;

    for (int i = 0; i < num_leds; i++) {
      // Distance behind the head (head moves downward, tail trails upward)
      float dist = (float)i - head_pos;
      if (dist < 0)
        dist += num_leds;

      // Brightness falls off behind the head
      float brightness = 0.0f;
      if (dist < tail_length) {
        brightness = 1.0f - (dist / tail_length);
        brightness = brightness * brightness; // smooth falloff
      }

      // Use palette - head gets bright end (index 255), tail fades toward 0
      uint8_t index = (uint8_t)(255.0f - dist / tail_length * 128.0f);
      RGB color =
          palette_sample(palette, index, (uint8_t)(brightness * 255), true);
      pixel(s, i, &color.r, &color.g, &color.b);
    }
  }
}

const Program programs[] = {
    {"Heartbeat", heartbeat_update, NULL, NULL},
    {"Rainbow", rainbow_update, NULL, NULL},
    {"Solid White", solid_white_update, NULL, NULL},
    {"Comet", comet_update, NULL, NULL},
};

const int NUM_PROGRAMS = 4;
