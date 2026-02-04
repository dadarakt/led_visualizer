#include "raylib.h"
#include "visualizer.h"

#include <dlfcn.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LIB_NAME "libvisualizer.so"

typedef struct VisualizerAPI {
  void (*init)(VisualizerState *);
  void (*update)(VisualizerState *);
  void (*draw)(VisualizerState *);
} VisualizerAPI;

static char lib_path[4096];

static void resolve_lib_path(void) {
  char exe[4096];
  ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (len <= 0) {
    snprintf(lib_path, sizeof(lib_path), "./%s", LIB_NAME);
    return;
  }
  exe[len] = '\0';
  snprintf(lib_path, sizeof(lib_path), "%s/%s", dirname(exe), LIB_NAME);
}

static time_t get_mtime(const char *path) {
  struct stat st;
  stat(path, &st);
  return st.st_mtime;
}

static VisualizerAPI load_visualizer(void **handle) {
  VisualizerAPI api = {0};

  *handle = dlopen(lib_path, RTLD_NOW);
  if (!*handle) {
    TraceLog(LOG_ERROR, dlerror());
    return api;
  }

  api.init = dlsym(*handle, "visualizer_init");
  api.update = dlsym(*handle, "visualizer_update");
  api.draw = dlsym(*handle, "visualizer_draw");

  return api;
}

int main(void) {
  resolve_lib_path();

  InitWindow(800, 450, "raylib hot reload (visualizer)");

  VisualizerState state = {0};
  void *visualizer_handle = NULL;
  VisualizerAPI visualizer = load_visualizer(&visualizer_handle);

  if (visualizer.init)
    visualizer.init(&state);

  time_t last_write = get_mtime(lib_path);

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    time_t write = get_mtime(lib_path);
    if (write != last_write) {
      TraceLog(LOG_INFO, "Reloading visualizer code");

      dlclose(visualizer_handle);
      visualizer = load_visualizer(&visualizer_handle);

      if (visualizer.init)
        visualizer.init(&state);

      last_write = write;
    }

    if (visualizer.update)
      visualizer.update(&state);

    if (visualizer.draw)
      visualizer.draw(&state);
  }

  dlclose(visualizer_handle);
  CloseWindow();
}
