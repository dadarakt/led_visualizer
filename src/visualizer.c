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

  memset(state->lights, 0, sizeof(state->lights));

  Color color;
  size_t offset;
  for (size_t i = 0; i < MAX_LIGHTS; ++i) {
    offset = 255 * ((float)i / MAX_LIGHTS);
    color = (Color){offset, 255 - offset, offset, 255};
    state->lights[i] =
        create_light(i, LIGHT_POINT, (Vector3){(float)i / 10, 0.5, 2},
                     Vector3Zero(), color, 0.02f, 0.5f, state->shader);
  }

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
    for (size_t i = 0; i < MAX_LIGHTS; ++i) {
      state->lights[i].enabled = !state->lights[i].enabled;
    }
  }

  for (int i = 0; i < MAX_LIGHTS; i++)
    update_light_values(state->shader, state->lights[i]);
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

  for (int i = 0; i < MAX_LIGHTS; i++) {
    state->lights[i].color.r += 1;
    state->lights[i].color.r %= 255;
    state->lights[i].color.g += 1;
    state->lights[i].color.g %= 255;
    state->lights[i].color.b += 1;
    state->lights[i].color.b %= 255;
    if (state->lights[i].enabled) {
      DrawSphereEx(state->lights[i].position, state->lights[i].radius, 8, 8,
                   state->lights[i].color);
    } else {
      DrawSphereWires(state->lights[i].position, state->lights[i].radius, 8, 8,
                      ColorAlpha(state->lights[i].color, 0.3f));
    }
  }

  DrawGrid(100, 1.0f);

  EndMode3D();

  DrawFPS(10, 10);

  DrawText("Use space to toggle lights", 10, 40, 20, DARKGRAY);

  EndDrawing();
}
