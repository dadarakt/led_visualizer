
#pragma once
#include "raylib.h"
#include <stdbool.h>

#define MAX_STRIPS 4
#define MAX_LEDS_PER_STRIP 144
#define SHADER_MAX_LIGHTS 144  // GPU uniform limit (~4096 components)

typedef enum { LIGHT_DIRECTIONAL = 0, LIGHT_POINT } LightType;

typedef struct {
  int type;
  bool enabled;
  Vector3 position;
  Vector3 target;
  Color color;
  float attenuation;
  int enabledLoc;
  int typeLoc;
  int positionLoc;
  int targetLoc;
  int colorLoc;
  int attenuationLoc;
  float radius;
} Light;

typedef struct {
  int num_leds;
  int base_index;           // offset into shader lights[] array
  int leds_per_light;       // how many LEDs averaged into one shader light
  int num_shader_lights;    // num_leds / leds_per_light
  Vector3 position;
  Vector3 rotation;
  float spacing;
  float intensity;
  float radius;
  Light leds[MAX_LEDS_PER_STRIP];           // visual LEDs (full resolution)
  Light shader_lights[MAX_LEDS_PER_STRIP];  // averaged lights sent to GPU
} LedStrip;

typedef struct VisualizerState {
  Camera camera;
  Shader shader;
  int num_strips;
  LedStrip strips[MAX_STRIPS];
} VisualizerState;

// Initialize state (load shader, create lights, set up camera)
void visualizer_init(VisualizerState *state);

// Update camera, input, light values
void visualizer_update(VisualizerState *state);

// Draw scene
void visualizer_draw(VisualizerState *state);
