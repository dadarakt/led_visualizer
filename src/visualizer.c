#include "visualizer.h"
#include "programs.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLSL_VERSION 330

// Light texture width: 2 pixels per light (pos+intensity, color+enabled)
// We cluster LEDs into groups to reduce shader light count
#define LIGHT_TEX_WIDTH (MAX_TOTAL_SHADER_LIGHTS * 2)

static void led_strip_create(LedStrip *strip, int num_leds, Vector3 position,
                             Vector3 rotation, float spacing, float intensity,
                             float radius) {
  strip->num_leds = num_leds;
  strip->position = position;
  strip->rotation = rotation;
  strip->spacing = spacing;
  strip->intensity = intensity;
  strip->radius = radius;
  memset(strip->leds, 0, sizeof(strip->leds));

  Matrix rot = MatrixRotateXYZ((Vector3){
      rotation.x * DEG2RAD, rotation.y * DEG2RAD, rotation.z * DEG2RAD});

  for (int i = 0; i < num_leds; i++) {
    Vector3 local_pos = {(float)i * spacing, 0.0f, 0.0f};
    Vector3 world_pos = Vector3Add(position, Vector3Transform(local_pos, rot));

    strip->leds[i] = (Light){
        .enabled = true,
        .position = world_pos,
        .color = (Color){0, 0, 0, 255},
        .radius = radius,
        .attenuation = intensity,
    };
  }
}

static void update_light_texture(VisualizerState *state) {
  // Each shader light uses 2 RGBA pixels: (pos.xyz, intensity), (color.rgb,
  // enabled) We cluster LEDS_PER_SHADER_LIGHT LEDs into one shader light
  static float lightData[LIGHT_TEX_WIDTH * 4];
  int numShaderLights = 0;

  for (int s = 0; s < state->num_strips; s++) {
    LedStrip *strip = &state->strips[s];
    int numGroups = strip->num_leds / LEDS_PER_SHADER_LIGHT;

    for (int g = 0; g < numGroups; g++) {
      int start = g * LEDS_PER_SHADER_LIGHT;

      // Average position and color across the group
      float px = 0, py = 0, pz = 0;
      float r = 0, gr = 0, b = 0;
      float intensity = 0;
      int enabledCount = 0;

      for (int j = 0; j < LEDS_PER_SHADER_LIGHT; j++) {
        Light *led = &strip->leds[start + j];
        px += led->position.x;
        py += led->position.y;
        pz += led->position.z;
        r += led->color.r / 255.0f;
        gr += led->color.g / 255.0f;
        b += led->color.b / 255.0f;
        intensity += led->attenuation;
        if (led->enabled)
          enabledCount++;
      }

      float inv = 1.0f / LEDS_PER_SHADER_LIGHT;
      int baseIdx = numShaderLights * 2 * 4;

      // Pixel 0: averaged position + summed intensity
      lightData[baseIdx + 0] = px * inv;
      lightData[baseIdx + 1] = py * inv;
      lightData[baseIdx + 2] = pz * inv;
      lightData[baseIdx + 3] = intensity; // sum, not average

      // Pixel 1: averaged color + enabled flag
      lightData[baseIdx + 4] = r * inv;
      lightData[baseIdx + 5] = gr * inv;
      lightData[baseIdx + 6] = b * inv;
      lightData[baseIdx + 7] = enabledCount > 0 ? 1.0f : 0.0f;

      numShaderLights++;
    }
  }

  rlUpdateTexture(state->lightTexture, 0, 0, LIGHT_TEX_WIDTH, 1,
                  RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, lightData);

  int numLightsLoc = GetShaderLocation(state->deferredShader, "numLights");
  SetShaderValue(state->deferredShader, numLightsLoc, &numShaderLights,
                 SHADER_UNIFORM_INT);
}

static void draw_person(Person *p, double time_ms) {
  float x = p->pos.x;
  float z = p->pos.z;
  float t = (float)(time_ms / 1000.0);
  float bob = 0.03f * sinf(t * 5.0f + p->phase);
  float y_off = bob;

  // Body
  DrawCube((Vector3){x, 1.1f + y_off, z}, 0.35f, 0.6f, 0.2f, GRAY);
  DrawSphere((Vector3){x, 1.55f + y_off, z}, 0.12f, GRAY);

  // Legs — bob knees slightly opposite to body
  float knee_bend = -bob * 0.5f;
  DrawCube((Vector3){x - 0.1f, 0.4f + knee_bend, z}, 0.12f, 0.8f, 0.15f, GRAY);
  DrawCube((Vector3){x + 0.1f, 0.4f - knee_bend, z}, 0.12f, 0.8f, 0.15f, GRAY);

  // Arms — slight offset from body bob
  float arm_bob = 0.03f * sinf(t * 5.0f + p->phase + 0.5f);
  DrawCube((Vector3){x - 0.27f, 1.05f + arm_bob, z}, 0.1f, 0.65f, 0.1f, GRAY);
  DrawCube((Vector3){x + 0.27f, 1.05f + arm_bob, z}, 0.1f, 0.65f, 0.1f, GRAY);
}

static void init_gbuffer(GBuffer *gb, int width, int height) {
  gb->framebuffer = rlLoadFramebuffer();
  if (gb->framebuffer == 0) {
    TraceLog(LOG_WARNING, "Failed to create G-buffer framebuffer");
    return;
  }

  rlEnableFramebuffer(gb->framebuffer);

  // Position texture (RGB16F for precision)
  gb->positionTexture = rlLoadTexture(NULL, width, height,
                                      RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16, 1);
  // Normal texture (RGB16F)
  gb->normalTexture = rlLoadTexture(NULL, width, height,
                                    RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16, 1);
  // Albedo texture (RGBA8)
  gb->albedoTexture = rlLoadTexture(NULL, width, height,
                                    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

  rlActiveDrawBuffers(3);

  rlFramebufferAttach(gb->framebuffer, gb->positionTexture,
                      RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
  rlFramebufferAttach(gb->framebuffer, gb->normalTexture,
                      RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);
  rlFramebufferAttach(gb->framebuffer, gb->albedoTexture,
                      RL_ATTACHMENT_COLOR_CHANNEL2, RL_ATTACHMENT_TEXTURE2D, 0);

  gb->depthRenderbuffer = rlLoadTextureDepth(width, height, true);
  rlFramebufferAttach(gb->framebuffer, gb->depthRenderbuffer,
                      RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

  if (!rlFramebufferComplete(gb->framebuffer)) {
    TraceLog(LOG_WARNING, "G-buffer framebuffer is not complete");
  }
}

void visualizer_init(VisualizerState *state) {
  if (state->gbufferShader.id != 0) {
    UnloadShader(state->gbufferShader);
    UnloadShader(state->deferredShader);
  }

  const char *appDir = GetApplicationDirectory();
  char vsPath[512], fsPath[512];

  // Load G-buffer shader
  snprintf(vsPath, sizeof(vsPath), "%sresources/shaders/glsl%i/gbuffer.vs",
           appDir, GLSL_VERSION);
  snprintf(fsPath, sizeof(fsPath), "%sresources/shaders/glsl%i/gbuffer.fs",
           appDir, GLSL_VERSION);
  state->gbufferShader = LoadShader(vsPath, fsPath);

  // Load deferred lighting shader
  snprintf(vsPath, sizeof(vsPath), "%sresources/shaders/glsl%i/deferred.vs",
           appDir, GLSL_VERSION);
  snprintf(fsPath, sizeof(fsPath), "%sresources/shaders/glsl%i/deferred.fs",
           appDir, GLSL_VERSION);
  state->deferredShader = LoadShader(vsPath, fsPath);
  state->deferredShader.locs[SHADER_LOC_VECTOR_VIEW] =
      GetShaderLocation(state->deferredShader, "viewPos");

  // Set up G-buffer texture samplers in deferred shader
  rlEnableShader(state->deferredShader.id);
  int texUnit0 = 0, texUnit1 = 1, texUnit2 = 2, texUnit3 = 3;
  SetShaderValue(state->deferredShader,
                 GetShaderLocation(state->deferredShader, "gPosition"),
                 &texUnit0, SHADER_UNIFORM_SAMPLER2D);
  SetShaderValue(state->deferredShader,
                 GetShaderLocation(state->deferredShader, "gNormal"), &texUnit1,
                 SHADER_UNIFORM_SAMPLER2D);
  SetShaderValue(state->deferredShader,
                 GetShaderLocation(state->deferredShader, "gAlbedo"), &texUnit2,
                 SHADER_UNIFORM_SAMPLER2D);
  SetShaderValue(state->deferredShader,
                 GetShaderLocation(state->deferredShader, "lightData"),
                 &texUnit3, SHADER_UNIFORM_SAMPLER2D);

  int ambientLoc = GetShaderLocation(state->deferredShader, "ambient");
  SetShaderValue(state->deferredShader, ambientLoc,
                 (float[4]){0.1f, 0.1f, 0.1f, 1.0f}, SHADER_UNIFORM_VEC4);

  state->lightTexWidth = LIGHT_TEX_WIDTH;
  int lightTexWidthLoc =
      GetShaderLocation(state->deferredShader, "lightTexWidth");
  SetShaderValue(state->deferredShader, lightTexWidthLoc, &state->lightTexWidth,
                 SHADER_UNIFORM_INT);
  rlDisableShader();

  // Initialize G-buffer
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();
  init_gbuffer(&state->gbuffer, screenWidth, screenHeight);

  // Create light data texture (1D texture as 1-row 2D texture)
  state->lightTexture = rlLoadTexture(
      NULL, LIGHT_TEX_WIDTH, 1, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
  rlTextureParameters(state->lightTexture, RL_TEXTURE_MIN_FILTER,
                      RL_TEXTURE_FILTER_NEAREST);
  rlTextureParameters(state->lightTexture, RL_TEXTURE_MAG_FILTER,
                      RL_TEXTURE_FILTER_NEAREST);
  rlTextureParameters(state->lightTexture, RL_TEXTURE_WRAP_S,
                      RL_TEXTURE_WRAP_CLAMP);
  rlTextureParameters(state->lightTexture, RL_TEXTURE_WRAP_T,
                      RL_TEXTURE_WRAP_CLAMP);

  float led_spacing = 1.0f / 143.0f;
  float led_radius = 0.004f;
  float led_intensity = 0.0015f;

  // 4 strips evenly spaced across the 5m back wall (X: -2.5 to 2.5)
  state->num_strips = 4;
  led_strip_create(
      &state->strips[0], MAX_LEDS_PER_STRIP, (Vector3){-1.875f, 1.0f, -2.95f},
      (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, led_intensity, led_radius);
  led_strip_create(
      &state->strips[1], MAX_LEDS_PER_STRIP, (Vector3){-0.625f, 1.0f, -2.95f},
      (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, led_intensity, led_radius);
  led_strip_create(
      &state->strips[2], MAX_LEDS_PER_STRIP, (Vector3){0.625f, 1.0f, -2.95f},
      (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, led_intensity, led_radius);
  led_strip_create(
      &state->strips[3], MAX_LEDS_PER_STRIP, (Vector3){1.875f, 1.0f, -2.95f},
      (Vector3){0.0f, 0.0f, 90.0f}, led_spacing, led_intensity, led_radius);

  state->active_program = 0;
  state->current_program = &programs[0];
  state->start_time = GetTime();
  state->time_ms = 0;

  state->camera_mode = CAMERA_CUSTOM;
  if (state->camera.fovy == 0) {
    state->camera.position = (Vector3){0.0f, 1.5f, -1.0f};
    state->camera.target = (Vector3){0.0f, 1.5f, -2.5f};
    state->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    state->camera.fovy = 70.0f;
    state->camera.projection = CAMERA_PERSPECTIVE;
  }

  // Place people to the sides, avoiding the center view
  srand(42);
  for (int i = 0; i < NUM_PEOPLE; i++) {
    float x = -2.0f + 4.0f * ((float)rand() / (float)RAND_MAX);
    // Push people away from center (|x| < 1.0)
    if (fabsf(x) < 1.0f) {
      x = (x < 0) ? x - 1.2f : x + 1.2f;
    }
    state->people[i].pos = (Vector3){
        x,
        0.0f,
        -2.0f + 3.5f * ((float)rand() / (float)RAND_MAX),
    };
    state->people[i].phase = 6.2832f * ((float)rand() / (float)RAND_MAX);
  }
}

void visualizer_update(VisualizerState *state) {
  // Update time
  state->time_ms = (GetTime() - state->start_time) * 1000.0;

  float cameraPos[3] = {state->camera.position.x, state->camera.position.y,
                        state->camera.position.z};
  SetShaderValue(state->deferredShader,
                 state->deferredShader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                 SHADER_UNIFORM_VEC3);

  if (IsKeyPressed(KEY_P)) {
    // Cleanup old program if it has a cleanup function
    if (state->current_program->cleanup) {
      state->current_program->cleanup();
    }
    state->active_program = (state->active_program + 1) % NUM_PROGRAMS;
    state->current_program = &programs[state->active_program];
    // Initialize new program if it has an init function
    if (state->current_program->init) {
      state->current_program->init();
    }
  }

  if (IsKeyPressed(KEY_T)) {
    for (int s = 0; s < state->num_strips; s++) {
      for (int i = 0; i < state->strips[s].num_leds; i++) {
        state->strips[s].leds[i].enabled = !state->strips[s].leds[i].enabled;
      }
    }
  }

  if (IsKeyPressed(KEY_X)) {
    if (state->camera_mode == CAMERA_FIRST_PERSON) {
      state->camera_mode = CAMERA_CUSTOM;
    } else {
      state->camera_mode = CAMERA_FIRST_PERSON;
    }
  }

  if (state->camera_mode == CAMERA_FIRST_PERSON) {
    UpdateCamera(&state->camera, CAMERA_FIRST_PERSON);
  }

  // Update LED colors via current program
  state->current_program->update(state->strips, state->num_strips,
                                 state->time_ms);

  update_light_texture(state);
}

static void draw_scene_geometry(VisualizerState *state) {
  // Room: 5m (X) x 3m (Y) x 6m (Z), centered at origin
  DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){5.0f, 6.0f}, WHITE);
  DrawCube((Vector3){0.0f, 3.0f, 0.0f}, 5.0f, 0.01f, 6.0f, GRAY);
  DrawCube((Vector3){0.0f, 1.5f, -3.0f}, 5.0f, 3.0f, 0.01f, GRAY);
  DrawCube((Vector3){-2.5f, 1.5f, 0.0f}, 0.01f, 3.0f, 6.0f, GRAY);
  DrawCube((Vector3){2.5f, 1.5f, 0.0f}, 0.01f, 3.0f, 6.0f, GRAY);
  DrawCube((Vector3){0.0f, 1.5f, 3.0f}, 5.0f, 3.0f, 0.01f, GRAY);

  for (int p = 0; p < NUM_PEOPLE; p++) {
    draw_person(&state->people[p], state->time_ms);
  }

  // LED strip housings
  for (int s = 0; s < state->num_strips; s++) {
    LedStrip *strip = &state->strips[s];
    float strip_len = (strip->num_leds - 1) * strip->spacing;
    Vector3 rot = strip->rotation;
    Matrix rotM = MatrixRotateXYZ(
        (Vector3){rot.x * DEG2RAD, rot.y * DEG2RAD, rot.z * DEG2RAD});
    Vector3 local_center = {strip_len * 0.5f, 0.0f, -0.005f};
    Vector3 housing_pos =
        Vector3Add(strip->position, Vector3Transform(local_center, rotM));
    Vector3 local_size = {strip_len + 0.01f, 0.015f, 0.003f};
    Vector3 sx = Vector3Transform((Vector3){local_size.x, 0, 0}, rotM);
    Vector3 sy = Vector3Transform((Vector3){0, local_size.y, 0}, rotM);
    Vector3 sz = Vector3Transform((Vector3){0, 0, local_size.z}, rotM);
    float wx = fabsf(sx.x) + fabsf(sy.x) + fabsf(sz.x);
    float wy = fabsf(sx.y) + fabsf(sy.y) + fabsf(sz.y);
    float wz = fabsf(sx.z) + fabsf(sy.z) + fabsf(sz.z);
    DrawCube(housing_pos, wx * 2, wy, wz, GRAY);
  }
}

void visualizer_draw(VisualizerState *state) {
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  BeginDrawing();

  // === PASS 1: Render geometry to G-buffer ===
  rlEnableFramebuffer(state->gbuffer.framebuffer);
  rlClearColor(0, 0, 0, 0);
  rlClearScreenBuffers();
  rlDisableColorBlend();

  BeginMode3D(state->camera);
  BeginShaderMode(state->gbufferShader);

  draw_scene_geometry(state);

  EndShaderMode();
  EndMode3D();

  rlEnableColorBlend();

  // === PASS 2: Deferred lighting to screen ===
  rlDisableFramebuffer();
  rlClearScreenBuffers();

  rlDisableColorBlend();
  rlEnableShader(state->deferredShader.id);

  // Bind G-buffer textures
  rlActiveTextureSlot(0);
  rlEnableTexture(state->gbuffer.positionTexture);
  rlActiveTextureSlot(1);
  rlEnableTexture(state->gbuffer.normalTexture);
  rlActiveTextureSlot(2);
  rlEnableTexture(state->gbuffer.albedoTexture);

  // Bind light data texture
  rlActiveTextureSlot(3);
  rlEnableTexture(state->lightTexture);

  // Draw fullscreen quad
  rlLoadDrawQuad();

  rlDisableShader();
  rlEnableColorBlend();

  // Copy depth buffer for correct occlusion of forward-rendered elements
  rlBindFramebuffer(RL_READ_FRAMEBUFFER, state->gbuffer.framebuffer);
  rlBindFramebuffer(RL_DRAW_FRAMEBUFFER, 0);
  rlBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth,
                    screenHeight, 0x00000100); // GL_DEPTH_BUFFER_BIT
  rlDisableFramebuffer();

  // === Forward pass: Draw LED spheres (emissive, not lit) ===
  BeginMode3D(state->camera);
  rlEnableShader(rlGetShaderIdDefault());

  for (int s = 0; s < state->num_strips; s++) {
    LedStrip *strip = &state->strips[s];
    for (int i = 0; i < strip->num_leds; i++) {
      Light *led = &strip->leds[i];
      if (led->enabled) {
        // Draw LED spheres at full brightness
        DrawSphereEx(led->position, led->radius, 4, 4, led->color);
      } else {
        DrawSphereWires(led->position, led->radius, 4, 4,
                        ColorAlpha(led->color, 0.1f));
      }
    }
  }

  rlDisableShader();
  EndMode3D();

  // === HUD ===
  DrawFPS(10, 10);
  DrawText(TextFormat("Program: %s (P to cycle)", state->current_program->name),
           10, 40, 20, DARKGRAY);
  DrawText("T to toggle lights", 10, 65, 20, DARKGRAY);
  DrawText(TextFormat("Lights: %d", state->num_strips * MAX_LEDS_PER_STRIP), 10,
           90, 20, DARKGRAY);

  EndDrawing();
}
