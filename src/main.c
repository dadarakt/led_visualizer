#include "raylib.h"
#include "visualizer.h"

#include <stdbool.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LIB_NAME "libvisualizer.so"
#define LIB_COPY_NAME "libvisualizer_live.so"

typedef struct VisualizerAPI {
  void (*init)(VisualizerState *);
  void (*update)(VisualizerState *);
  void (*draw)(VisualizerState *);
} VisualizerAPI;

static char lib_path[4096];
static char lib_copy_path[4096];

static void resolve_lib_path(void) {
  char exe[4096];
  ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (len <= 0) {
    snprintf(lib_path, sizeof(lib_path), "./%s", LIB_NAME);
    return;
  }
  exe[len] = '\0';
  char *dir = dirname(exe);
  snprintf(lib_path, sizeof(lib_path), "%s/%s", dir, LIB_NAME);
  snprintf(lib_copy_path, sizeof(lib_copy_path), "%s/%s", dir, LIB_COPY_NAME);
}

static time_t get_mtime(const char *path) {
  struct stat st;
  stat(path, &st);
  return st.st_mtime;
}

static bool copy_file(const char *src, const char *dst) {
  int src_fd = open(src, O_RDONLY);
  if (src_fd < 0)
    return false;

  int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
  if (dst_fd < 0) {
    close(src_fd);
    return false;
  }

  char buf[65536];
  ssize_t n;
  while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
    if (write(dst_fd, buf, n) != n) {
      close(src_fd);
      close(dst_fd);
      return false;
    }
  }

  close(src_fd);
  close(dst_fd);
  return n == 0;
}

static VisualizerAPI load_visualizer(void **handle) {
  VisualizerAPI api = {0};

  if (!copy_file(lib_path, lib_copy_path)) {
    TraceLog(LOG_ERROR, "Failed to copy %s", lib_path);
    return api;
  }

  *handle = dlopen(lib_copy_path, RTLD_NOW);
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
      // Wait for the linker to finish writing the .so file.
      // The mtime changes as soon as the linker starts writing,
      // but the file may not be complete yet.
      usleep(100000);

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
