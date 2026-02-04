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
                             int num_leds, int leds_per_light,
                             Vector3 position, Vector3 rotation,
                             float spacing, float intensity, float radius) {
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

  Matrix rot = MatrixRotateXYZ(
      (Vector3){rotation.x * DEG2RAD, rotation.y * DEG2RAD, rotation.z * DEG2RAD});

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
    strip->shader_lights[g] = create_light(
        shader_index, LIGHT_POINT, center, Vector3Zero(),
        strip->leds[start].color, radius, intensity, shader);
  }
}

static void led_strip_update(LedStrip *strip, Shader shader) {
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
      if (led->enabled) any_enabled = true;
    }
    strip->shader_lights[g].color = (Color){r / lpl, gr / lpl, b / lpl, a / lpl};
    strip->shader_lights[g].enabled = any_enabled;

    update_light_values(shader, strip->shader_lights[g]);
  }
}

static void led_strip_draw(LedStrip *strip) {
  for (int i = 0; i < strip->num_leds; i++) {
    Light *led = &strip->leds[i];
    led->color.r += 1;
    led->color.r %= 255;
    led->color.g += 1;
    led->color.g %= 255;
    led->color.b += 1;
    led->color.b %= 255;
    if (led->enabled) {
      DrawSphereEx(led->position, led->radius, 8, 8, led->color);
    } else {
      DrawSphereWires(led->position, led->radius, 8, 8,
                      ColorAlpha(led->color, 0.3f));
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

  state->num_strips = 2;
  // leds_per_light=8: average every 8 LEDs into one shader light (18 per strip)
  led_strip_create(&state->strips[0], state->shader, 0, MAX_LEDS_PER_STRIP,
                   8, (Vector3){0.0f, 0.5f, 2.0f}, (Vector3){0.0f, 0.0f, 0.0f},
                   0.1f, 0.1f, 0.02f);
  led_strip_create(&state->strips[1], state->shader, 18, MAX_LEDS_PER_STRIP,
                   8, (Vector3){0.0f, 1.5f, 0.0f}, (Vector3){0.0f, 45.0f, 0.0f},
                   0.1f, 0.1f, 0.02f);

  if (state->camera.fovy == 0) {
    state->camera.position = (Vector3){2.0f, 4.0f, 6.0f};
    state->camera.target = (Vector3){0.0f, 0.5f, 0.0f};
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

  if (IsKeyPressed(KEY_SPACE)) {
    for (int s = 0; s < state->num_strips; s++) {
      for (int i = 0; i < state->strips[s].num_leds; i++) {
        state->strips[s].leds[i].enabled = !state->strips[s].leds[i].enabled;
      }
    }
  }

  for (int s = 0; s < state->num_strips; s++) {
    led_strip_update(&state->strips[s], state->shader);
  }
}

void visualizer_draw(VisualizerState *state) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  BeginMode3D(state->camera);
  BeginShaderMode(state->shader);

  DrawPlane(Vector3Zero(), (Vector2){100.0f, 100.0f}, GRAY);
  DrawCube((Vector3){0, 0, 0}, 2.0f, 4.0f, 2.0f, GRAY);
  DrawCube((Vector3){6, 0, 0}, 2.0f, 4.0f, 2.0f, GRAY);
  DrawCube((Vector3){12, 0, 0}, 2.0f, 4.0f, 2.0f, GRAY);
  DrawCube((Vector3){18, 0, 0}, 2.0f, 4.0f, 2.0f, GRAY);

  EndShaderMode();

  for (int s = 0; s < state->num_strips; s++) {
    led_strip_draw(&state->strips[s]);
  }

  DrawGrid(100, 1.0f);

  EndMode3D();

  DrawFPS(10, 10);

  DrawText("Use space to toggle lights", 10, 40, 20, DARKGRAY);

  EndDrawing();
}
