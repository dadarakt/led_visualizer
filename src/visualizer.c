#include "visualizer.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>

#define GLSL_VERSION 330

static void update_light_values(Shader shader, Light light) {
  SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
  SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

  float position[3] = {light.position.x, light.position.y, light.position.z};
  SetShaderValue(shader, light.positionLoc, position, SHADER_UNIFORM_VEC3);

  float target[3] = {light.target.x, light.target.y, light.target.z};
  SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);

  float color[4] = {
      (float)light.color.r / 255.0f, (float)light.color.g / 255.0f,
      (float)light.color.b / 255.0f, (float)light.color.a / 255.0f};
  SetShaderValue(shader, light.colorLoc, color, SHADER_UNIFORM_VEC4);
  SetShaderValue(shader, light.attenuationLoc, &light.attenuation,
                 SHADER_UNIFORM_FLOAT);
}

static Light create_light(int index, int type, Vector3 position, Vector3 target,
                          Color color, float radius, float intensity,
                          Shader shader) {
  Light light = {0};
  light.enabled = true;
  light.type = type;
  light.position = position;
  light.target = target;
  light.color = color;
  light.radius = radius;
  light.attenuation = intensity;

  light.enabledLoc =
      GetShaderLocation(shader, TextFormat("lights[%i].enabled", index));
  light.typeLoc =
      GetShaderLocation(shader, TextFormat("lights[%i].type", index));
  light.positionLoc =
      GetShaderLocation(shader, TextFormat("lights[%i].position", index));
  light.targetLoc =
      GetShaderLocation(shader, TextFormat("lights[%i].target", index));
  light.colorLoc =
      GetShaderLocation(shader, TextFormat("lights[%i].color", index));
  light.attenuationLoc =
      GetShaderLocation(shader, TextFormat("lights[%i].intensity", index));

  update_light_values(shader, light);
  return light;
}

static void led_strip_create(LedStrip *strip, Shader shader, int base_index,
                             int num_leds, int leds_per_light, Vector3 position,
                             Vector3 rotation, float spacing, float intensity,
                             float radius) {
  strip->num_leds = num_leds;
  strip->base_index = base_index;
  strip->leds_per_light = leds_per_light;
  strip->num_shader_lights = num_leds / leds_per_light;
  strip->position = position;
  strip->rotation = rotation;
  strip->spacing = spacing;
  strip->intensity = intensity;
  strip->radius = radius;
  memset(strip->leds, 0, sizeof(strip->leds));
  memset(strip->shader_lights, 0, sizeof(strip->shader_lights));

  Matrix rot = MatrixRotateXYZ((Vector3){
      rotation.x * DEG2RAD, rotation.y * DEG2RAD, rotation.z * DEG2RAD});

  // Create visual LEDs at full resolution (no shader locations needed)
  for (int i = 0; i < num_leds; i++) {
    Vector3 local_pos = {(float)i * spacing, 0.0f, 0.0f};
    Vector3 world_pos = Vector3Add(position, Vector3Transform(local_pos, rot));

    size_t offset = 255 * ((float)i / num_leds);
    Color color = {offset, 255 - offset, offset, 255};

    strip->leds[i] = (Light){
        .enabled = true,
        .type = LIGHT_POINT,
        .position = world_pos,
        .target = Vector3Zero(),
        .color = color,
        .radius = radius,
        .attenuation = intensity,
    };
  }

  // Create shader lights (averaged groups) with shader uniform locations
  for (int g = 0; g < strip->num_shader_lights; g++) {
    int start = g * leds_per_light;
    Vector3 center = {0};
    for (int j = 0; j < leds_per_light; j++) {
      center = Vector3Add(center, strip->leds[start + j].position);
    }
    center = Vector3Scale(center, 1.0f / leds_per_light);

    int shader_index = base_index + g;
    strip->shader_lights[g] =
        create_light(shader_index, LIGHT_POINT, center, Vector3Zero(),
                     strip->leds[start].color, radius, intensity, shader);
  }
}

static void led_strip_update(LedStrip *strip, Shader shader, long t) {
  int lpl = strip->leds_per_light;

  for (int g = 0; g < strip->num_shader_lights; g++) {
    int start = g * lpl;

    // Average color across the group
    int r = 0, gr = 0, b = 0, a = 0;
    bool any_enabled = false;
    for (int j = 0; j < lpl; j++) {
      Light *led = &strip->leds[start + j];
      r += led->color.r;
      gr += led->color.g;
      b += led->color.b;
      a += led->color.a;
      if (led->enabled)
        any_enabled = true;
    }
    strip->shader_lights[g].color =
        (Color){r / lpl, gr / lpl, b / lpl, a / lpl};
    strip->shader_lights[g].enabled = any_enabled;

    update_light_values(shader, strip->shader_lights[g]);
  }
}

static void led_strip_draw(LedStrip *strip, long t) {
  // Draw housing behind the LEDs
  float strip_len = (strip->num_leds - 1) * strip->spacing;
  Vector3 rot = strip->rotation;
  Matrix rotM = MatrixRotateXYZ(
      (Vector3){rot.x * DEG2RAD, rot.y * DEG2RAD, rot.z * DEG2RAD});

  // Center of the strip in local space, offset slightly behind (negative Z)
  Vector3 local_center = {strip_len * 0.5f, 0.0f, -0.005f};
  Vector3 housing_pos =
      Vector3Add(strip->position, Vector3Transform(local_center, rotM));

  // Housing size: length of strip x narrow width x thin depth
  Vector3 local_size = {strip_len + 0.01f, 0.015f, 0.003f};
  // Rotate the size axes to get world-aligned cube dimensions
  Vector3 sx = Vector3Transform((Vector3){local_size.x, 0, 0}, rotM);
  Vector3 sy = Vector3Transform((Vector3){0, local_size.y, 0}, rotM);
  Vector3 sz = Vector3Transform((Vector3){0, 0, local_size.z}, rotM);
  float wx = fabsf(sx.x) + fabsf(sy.x) + fabsf(sz.x);
  float wy = fabsf(sx.y) + fabsf(sy.y) + fabsf(sz.y);
  float wz = fabsf(sx.z) + fabsf(sy.z) + fabsf(sz.z);

  DrawCube(housing_pos, wx, wy, wz, GRAY);

  for (int i = 0; i < strip->num_leds; i++) {
    Light *led = &strip->leds[i];
    // Pulse red-to-blue across the strip
    float pos = (float)i / (float)(strip->num_leds - 1); // 0..1 along strip
    float pulse = 0.5f + 0.5f * sinf((float)t * 0.03f - pos * 6.0f);

    // Heartbeat intensity: double-bump (lub-dub) at ~72 bpm
    float beat_phase = fmodf((float)t * 0.02f, 6.2832f) / 6.2832f; // 0..1
    float lub = expf(-powf((beat_phase - 0.15f) * 12.0f, 2.0f));
    float dub = expf(-powf((beat_phase - 0.35f) * 12.0f, 2.0f));
    float heartbeat = 0.6f + 0.4f * fmaxf(lub, dub);

    led->color.r = (unsigned char)(255.0f * (1.0f - pulse) * heartbeat);
    led->color.g = 0;
    led->color.b = (unsigned char)(255.0f * pulse * heartbeat);

    if (led->enabled) {
      DrawSphereEx(led->position, led->radius, 4, 4,
                   ColorAlpha(led->color, 0.4f));
    } else {
      DrawSphereWires(led->position, led->radius, 4, 4,
                      ColorAlpha(led->color, 0.1f));
    }
  }
}

void visualizer_init(VisualizerState *state) {
  if (state->shader.id != 0) {
    UnloadShader(state->shader);
  }

  const char *appDir = GetApplicationDirectory();
  char vsPath[512], fsPath[512];
  snprintf(vsPath, sizeof(vsPath), "%sresources/shaders/glsl%i/lighting.vs",
           appDir, GLSL_VERSION);
  snprintf(fsPath, sizeof(fsPath), "%sresources/shaders/glsl%i/lighting.fs",
           appDir, GLSL_VERSION);

  state->shader = LoadShader(vsPath, fsPath);
  state->shader.locs[SHADER_LOC_VECTOR_VIEW] =
      GetShaderLocation(state->shader, "viewPos");

  int ambientLoc = GetShaderLocation(state->shader, "ambient");
  SetShaderValue(state->shader, ambientLoc, (float[4]){0.1f, 0.1f, 0.1f, 1.0f},
                 SHADER_UNIFORM_VEC4);

  float led_spacing = 1.0f / 143.0f; // 144 LEDs over 1m
  float led_radius = 0.003f;

  state->num_strips = 4;
  // Two strips against the back wall (Z=-2.95), pointing upward (90° on Z),
  // equidistant from corners: X = -5/6 and X = 5/6, starting at Y=1.0
  led_strip_create(&state->strips[0], state->shader, 0, MAX_LEDS_PER_STRIP, 8,
                   (Vector3){-10.0f / 6.0f, 1.0f, -2.95f},
                   (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, 0.30f,
                   led_radius);
  led_strip_create(&state->strips[1], state->shader, 18, MAX_LEDS_PER_STRIP, 8,
                   (Vector3){-5.0f / 6.0f, 1.0f, -2.95f},
                   (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, 0.30f,
                   led_radius);
  led_strip_create(&state->strips[2], state->shader, 36, MAX_LEDS_PER_STRIP, 8,
                   (Vector3){5.0f / 6.0f, 1.0f, -2.95f},
                   (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, 0.30f,
                   led_radius);
  led_strip_create(&state->strips[3], state->shader, 54, MAX_LEDS_PER_STRIP, 8,
                   (Vector3){10.0f / 6.0f, 1.0f, -2.95f},
                   (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, 0.30f,
                   led_radius);

  if (state->camera.fovy == 0) {
    state->camera.position = (Vector3){0.5f, 1.6f, 2.5f};
    state->camera.target = (Vector3){0.0f, 1.0f, -1.0f};
    state->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    state->camera.fovy = 45.0f;
    state->camera.projection = CAMERA_PERSPECTIVE;
  }
}

void visualizer_update(VisualizerState *state) {
  UpdateCamera(&state->camera, CAMERA_FREE);

  float cameraPos[3] = {state->camera.position.x, state->camera.position.y,
                        state->camera.position.z};
  SetShaderValue(state->shader, state->shader.locs[SHADER_LOC_VECTOR_VIEW],
                 cameraPos, SHADER_UNIFORM_VEC3);

  if (IsKeyPressed(KEY_T)) {
    for (int s = 0; s < state->num_strips; s++) {
      for (int i = 0; i < state->strips[s].num_leds; i++) {
        state->strips[s].leds[i].enabled = !state->strips[s].leds[i].enabled;
      }
    }
  }

  for (int s = 0; s < state->num_strips; s++) {
    led_strip_update(&state->strips[s], state->shader, state->t);
  }

  state->t += 1;
}

void visualizer_draw(VisualizerState *state) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  BeginMode3D(state->camera);
  BeginShaderMode(state->shader);

  // Room: 5m (X) x 3m (Y) x 6m (Z), centered at origin
  // Floor
  DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){5.0f, 6.0f}, GRAY);
  // Ceiling
  DrawCube((Vector3){0.0f, 3.0f, 0.0f}, 5.0f, 0.01f, 6.0f, DARKGRAY);
  // Back wall (Z = -3)
  DrawCube((Vector3){0.0f, 1.5f, -3.0f}, 5.0f, 3.0f, 0.01f, DARKGRAY);
  // Left wall (X = -2.5)
  DrawCube((Vector3){-2.5f, 1.5f, 0.0f}, 0.01f, 3.0f, 6.0f, DARKGRAY);
  // Right wall (X = 2.5)
  DrawCube((Vector3){2.5f, 1.5f, 0.0f}, 0.01f, 3.0f, 6.0f, DARKGRAY);
  // Front wall (Z = 3) — behind the camera
  DrawCube((Vector3){0.0f, 1.5f, 3.0f}, 5.0f, 3.0f, 0.01f, DARKGRAY);

  // Person (~1.8m tall)
  // Torso
  DrawCube((Vector3){0.0f, 1.1f, 0.0f}, 0.35f, 0.6f, 0.2f, GRAY);
  // Head
  DrawSphere((Vector3){0.0f, 1.55f, 0.0f}, 0.12f, GRAY);
  // Left leg
  DrawCube((Vector3){-0.1f, 0.4f, 0.0f}, 0.12f, 0.8f, 0.15f, GRAY);
  // Right leg
  DrawCube((Vector3){0.1f, 0.4f, 0.0f}, 0.12f, 0.8f, 0.15f, GRAY);
  // Left arm
  DrawCube((Vector3){-0.27f, 1.05f, 0.0f}, 0.1f, 0.65f, 0.1f, GRAY);
  // Right arm
  DrawCube((Vector3){0.27f, 1.05f, 0.0f}, 0.1f, 0.65f, 0.1f, GRAY);

  EndShaderMode();

  for (int s = 0; s < state->num_strips; s++) {
    led_strip_draw(&state->strips[s], state->t);
  }

  EndMode3D();

  DrawFPS(10, 10);

  DrawText("Use T to toggle lights", 10, 40, 20, DARKGRAY);

  EndDrawing();
}
