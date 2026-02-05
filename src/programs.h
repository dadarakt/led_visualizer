#pragma once

#include "visualizer.h"

// Program interface
struct Program {
  const char *name;
  void (*update)(LedStrip *strips, int num_strips, double time_ms);
  void (*init)(void);    // optional: called when program becomes active
  void (*cleanup)(void); // optional: called when switching away
};

// Program registry
extern const Program programs[];
extern const int NUM_PROGRAMS;
