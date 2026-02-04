#include "visualizer.h"
#include <math.h>
#include <stdlib.h>

void visualizer_init(VisualizerState *state) {
  if (!state->bullets) {
    state->bullets = RL_CALLOC(MAX_BULLETS, sizeof(Bullet));
  }

  state->bulletRadius = 10;
  state->bulletSpeed = 3.0f;
  state->bulletRows = 6;
  state->spawnCooldown = 2;
  state->spawnCooldownTimer = state->spawnCooldown;
  state->angleIncrement = 5;
  state->drawInPerformanceMode = true;
  state->baseDirection = 0;
  state->magicCircleRotation = 0;

  if (state->bulletTexture.id == 0) {
    state->bulletTexture = LoadRenderTexture(24, 24);
    BeginTextureMode(state->bulletTexture);
    DrawCircle(12, 12, (float)state->bulletRadius, WHITE);
    DrawCircleLines(12, 12, (float)state->bulletRadius, BLACK);
    EndTextureMode();
  }
}

void visualizer_update(VisualizerState *state) {
  // Spawn bullets
  state->spawnCooldownTimer--;
  if (state->spawnCooldownTimer < 0) {
    state->spawnCooldownTimer = state->spawnCooldown;
    float degreesPerRow = 360.0f / state->bulletRows;

    for (int row = 0; row < state->bulletRows; row++) {
      if (state->bulletCount >= MAX_BULLETS)
        break;

      Bullet *b = &state->bullets[state->bulletCount++];
      b->position = (Vector2){400, 225}; // screen center
      b->disabled = false;
      b->color = (row % 2) ? GREEN : SKYBLUE;

      float bulletDirection = state->baseDirection + degreesPerRow * row;
      b->acceleration =
          (Vector2){state->bulletSpeed * cosf(bulletDirection * DEG2RAD),
                    state->bulletSpeed * sinf(bulletDirection * DEG2RAD)};
    }

    state->baseDirection += state->angleIncrement;
  }

  // Update bullet positions
  for (int i = 0; i < state->bulletCount; i++) {
    Bullet *b = &state->bullets[i];
    if (!b->disabled) {
      b->position.x += b->acceleration.x;
      b->position.y += b->acceleration.y;

      if (b->position.x < -state->bulletRadius * 2 ||
          b->position.x > 800 + state->bulletRadius * 2 ||
          b->position.y < -state->bulletRadius * 2 ||
          b->position.y > 450 + state->bulletRadius * 2) {
        b->disabled = true;
        state->bulletDisabledCount++;
      }
    }
  }

  // Input adjustments (rows, speed, cooldown, etc.)
  if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) &&
      (state->bulletRows < 359))
    state->bulletRows++;
  if ((IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) &&
      (state->bulletRows > 1))
    state->bulletRows--;
  if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
    state->bulletSpeed += 0.25f;
  if ((IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) &&
      (state->bulletSpeed > 0.5f))
    state->bulletSpeed -= 0.25f;
  if (IsKeyPressed(KEY_Z) && (state->spawnCooldown > 1))
    state->spawnCooldown--;
  if (IsKeyPressed(KEY_X))
    state->spawnCooldown++;
  if (IsKeyPressed(KEY_ENTER))
    state->drawInPerformanceMode = !state->drawInPerformanceMode;
  if (IsKeyPressed(KEY_C)) {
    state->bulletCount = 0;
    state->bulletDisabledCount = 0;
  }

  state->magicCircleRotation++;
}

void visualizer_draw(VisualizerState *state) {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  // Draw bullets
  if (state->drawInPerformanceMode) {
    for (int i = 0; i < state->bulletCount; i++) {
      Bullet *b = &state->bullets[i];
      if (!b->disabled) {
        DrawTexture(
            state->bulletTexture.texture,
            (int)(b->position.x - state->bulletTexture.texture.width * 0.5f),
            (int)(b->position.y - state->bulletTexture.texture.height * 0.5f),
            b->color);
      }
    }
  } else {
    for (int i = 0; i < state->bulletCount; i++) {
      Bullet *b = &state->bullets[i];
      if (!b->disabled) {
        DrawCircleV(b->position, (float)state->bulletRadius, b->color);
        DrawCircleLinesV(b->position, (float)state->bulletRadius, BLACK);
      }
    }
  }

  // Draw magic circle
  DrawRectanglePro((Rectangle){400, 225, 120, 120}, (Vector2){60, 60},
                   state->magicCircleRotation, PURPLE);
  DrawRectanglePro((Rectangle){400, 225, 120, 120}, (Vector2){60, 60},
                   state->magicCircleRotation + 45, PURPLE);
  DrawCircleLines(400, 225, 70, BLACK);
  DrawCircleLines(400, 225, 50, BLACK);
  DrawCircleLines(400, 225, 30, BLACK);

  EndDrawing();
}
