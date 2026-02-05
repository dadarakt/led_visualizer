#include "programs.h"
#include <math.h>
#include <stddef.h>

static void heartbeat_update(LedStrip *strips, int num_strips, double time_ms) {
  double t = time_ms / 1000.0;

  double beat_period = 60.0 / 72.0; // ~72 bpm
  double beat_phase = fmod(t, beat_period) / beat_period;
  double lub = exp(-pow((beat_phase - 0.15) * 12.0, 2.0));
  double dub = exp(-pow((beat_phase - 0.35) * 12.0, 2.0));
  float heartbeat = 0.6f + 0.4f * fmax(lub, dub);

  for (int s = 0; s < num_strips; s++) {
    LedStrip *strip = &strips[s];
    for (int i = 0; i < strip->num_leds; i++) {
      Light *led = &strip->leds[i];
      float pulse =
          0.5f + 0.5f * sinf((float)(t * 2.0) - led->position.x * 2.0f);

      led->color.r = (unsigned char)(255.0f * (1.0f - pulse) * heartbeat);
      led->color.g = 0;
      led->color.b = (unsigned char)(255.0f * pulse * heartbeat);
    }
  }
}

static void rainbow_update(LedStrip *strips, int num_strips, double time_ms) {
  float t = (float)(time_ms / 1000.0);

  for (int s = 0; s < num_strips; s++) {
    LedStrip *strip = &strips[s];
    for (int i = 0; i < strip->num_leds; i++) {
      Light *led = &strip->leds[i];
      float phase = t * 3.0f;
      float offset = (float)i / (float)strip->num_leds * 6.2832f;

      led->color.r = (unsigned char)(127.5f + 127.5f * sinf(phase + offset));
      led->color.g =
          (unsigned char)(127.5f + 127.5f * sinf(phase + offset + 2.094f));
      led->color.b =
          (unsigned char)(127.5f + 127.5f * sinf(phase + offset + 4.189f));
    }
  }
}

static void solid_white_update(LedStrip *strips, int num_strips,
                               double time_ms) {
  (void)time_ms;

  for (int s = 0; s < num_strips; s++) {
    LedStrip *strip = &strips[s];
    for (int i = 0; i < strip->num_leds; i++) {
      Light *led = &strip->leds[i];
      led->color.r = 255;
      led->color.g = 255;
      led->color.b = 255;
    }
  }
}

// --- Program Registry ---

const Program programs[] = {
    {"Heartbeat", heartbeat_update, NULL, NULL},
    {"Rainbow", rainbow_update, NULL, NULL},
    {"Solid White", solid_white_update, NULL, NULL},
};

const int NUM_PROGRAMS = 3;
