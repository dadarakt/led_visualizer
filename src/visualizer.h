
#pragma once
#include "raylib.h"
#include <stdbool.h>

#define MAX_LIGHTS 144

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

typedef struct VisualizerState {
  Camera camera;
  Shader shader;
  Light lights[MAX_LIGHTS];
} VisualizerState;

// Initialize state (load shader, create lights, set up camera)
void visualizer_init(VisualizerState *state);

// Update camera, input, light values
void visualizer_update(VisualizerState *state);

// Draw scene
void visualizer_draw(VisualizerState *state);
