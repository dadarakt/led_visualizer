
#pragma once
#include "raylib.h"
#include <stdbool.h>

#define MAX_BULLETS 500000

typedef struct Bullet {
  Vector2 position;
  Vector2 acceleration;
  bool disabled;
  Color color;
} Bullet;

typedef struct VisualizerState {
  Bullet *bullets;
  int bulletCount;
  int bulletDisabledCount;

  int bulletRadius;
  float bulletSpeed;
  int bulletRows;

  float baseDirection;
  int angleIncrement;
  float spawnCooldown;
  float spawnCooldownTimer;

  float magicCircleRotation;

  RenderTexture bulletTexture;
  bool drawInPerformanceMode;
} VisualizerState;

// Initialize state (allocate bullets, textures, etc.)
void visualizer_init(VisualizerState *state);

// Update bullets, input, etc.
void visualizer_update(VisualizerState *state);

// Draw bullets and UI
void visualizer_draw(VisualizerState *state);
