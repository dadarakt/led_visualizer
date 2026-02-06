
#pragma once
#include "palette.h"
#include "programs.h"
#include "raylib.h"
#include <stdbool.h>

#define MAX_STRIPS 4
#define NUM_PEOPLE 10
#define MAX_LEDS_PER_STRIP 144
#define LEDS_PER_SHADER_LIGHT 8
#define MAX_SHADER_LIGHTS_PER_STRIP (MAX_LEDS_PER_STRIP / LEDS_PER_SHADER_LIGHT)
#define MAX_TOTAL_SHADER_LIGHTS (MAX_STRIPS * MAX_SHADER_LIGHTS_PER_STRIP)

// G-Buffer for deferred rendering
typedef struct {
  unsigned int framebuffer;
  unsigned int positionTexture;
  unsigned int normalTexture;
  unsigned int albedoTexture;
  unsigned int depthRenderbuffer;
} GBuffer;

typedef struct {
  bool enabled;
  Vector3 position;
  Color color;
  float attenuation;
  float radius;
} Light;

typedef struct {
  int num_leds;
  Vector3 position;
  Vector3 rotation;
  float spacing;
  float intensity;
  float radius;
  Light leds[MAX_LEDS_PER_STRIP];
} LedStrip;

typedef struct {
  Vector3 pos;
  float phase; // bob phase offset
} Person;

typedef struct VisualizerState {
  Camera camera;
  CameraMode camera_mode;
  Shader gbufferShader;
  Shader deferredShader;
  GBuffer gbuffer;
  unsigned int lightTexture;
  int lightTexWidth;
  int num_strips;
  LedStrip strips[MAX_STRIPS];
  double start_time;
  double time_ms;
  double last_frame_time;
  double smoothed_delta;
  // Programs (loaded dynamically)
  const Program *programs;
  int num_programs;
  int active_program;
  const Program *current_program;
  int active_palette;
  const Palette16 *current_palette;
  Person people[NUM_PEOPLE];
  bool simple_render_mode;
} VisualizerState;

// Initialize state (load shaders, set up camera)
void visualizer_init(VisualizerState *state);

// Configure strips from StripDef array (call after init, and on hot-reload)
void visualizer_configure_strips(VisualizerState *state,
                                 const StripDef *strip_setup, int num_strips);

// Update camera, input, light values
void visualizer_update(VisualizerState *state);

// Draw scene
void visualizer_draw(VisualizerState *state);
