#include "programs.h"
#include <math.h>
#include <stddef.h>

static void heartbeat_update(int num_strips, int num_leds, double time_ms,
                             PixelFunc pixel) {
  double t = time_ms / 1000.0;

  double beat_period = 60.0 / 72.0; // ~72 bpm
  double beat_phase = fmod(t, beat_period) / beat_period;
  double lub = exp(-pow((beat_phase - 0.15) * 12.0, 2.0));
  double dub = exp(-pow((beat_phase - 0.35) * 12.0, 2.0));
  float heartbeat = 0.6f + 0.4f * fmax(lub, dub);

  for (int s = 0; s < num_strips; s++) {
    for (int i = 0; i < num_leds; i++) {
      // Use led index for horizontal gradient (0 to 1 across strip)
      float pos = (float)i / (float)num_leds;
      float pulse = 0.5f + 0.5f * sinf((float)(t * 2.0) - pos * 6.28f);

      uint8_t r = (uint8_t)(255.0f * (1.0f - pulse) * heartbeat);
      uint8_t g = 0;
      uint8_t b = (uint8_t)(255.0f * pulse * heartbeat);
      pixel(s, i, &r, &g, &b);
    }
  }
}

static void rainbow_update(int num_strips, int num_leds, double time_ms,
                           PixelFunc pixel) {
  float t = (float)(time_ms / 1000.0);

  for (int s = 0; s < num_strips; s++) {
    for (int i = 0; i < num_leds; i++) {
      float phase = t * 3.0f;
      float offset = (float)i / (float)num_leds * 6.2832f;

      uint8_t r = (uint8_t)(127.5f + 127.5f * sinf(phase + offset));
      uint8_t g = (uint8_t)(127.5f + 127.5f * sinf(phase + offset + 2.094f));
      uint8_t b = (uint8_t)(127.5f + 127.5f * sinf(phase + offset + 4.189f));
      pixel(s, i, &r, &g, &b);
    }
  }
}

static void solid_white_update(int num_strips, int num_leds, double time_ms,
                               PixelFunc pixel) {
  (void)time_ms;

  for (int s = 0; s < num_strips; s++) {
    for (int i = 0; i < num_leds; i++) {
      uint8_t r = 255, g = 255, b = 255;
      pixel(s, i, &r, &g, &b);
    }
  }
}

static void comet_update(int num_strips, int num_leds, double time_ms,
                         PixelFunc pixel) {
  float t = (float)(time_ms / 1000.0);
  float tail_length = 25.0f;

  for (int s = 0; s < num_strips; s++) {
    // Pseudo-random per-strip values based on strip index
    unsigned int seed = (unsigned int)(s * 2654435761u);
    float speed = 0.3f + (float)(seed % 100) / 200.0f;        // 0.3 - 0.8
    float phase_offset = (float)(seed % 1000) / 1000.0f;      // 0.0 - 1.0

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

      uint8_t val = (uint8_t)(255.0f * brightness);
      pixel(s, i, &val, &val, &val);
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
