#include "raylib.h"
#include "visualizer.h"
#include "programs.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#define LIB_EXT ".dylib"
#else
#define LIB_EXT ".so"
#endif

// Paths resolved at startup
static char exe_dir[4096];
static char sdk_header_path[4096];
static char sdk_source_path[4096];
static char compiled_lib_path[4096];
static char source_file_path[4096];

typedef struct {
  void *handle;
  const Program *programs;
  const int *num_programs;
} LoadedPrograms;

static void resolve_exe_dir(void) {
  char exe[4096];
  bool found = false;

#ifdef __APPLE__
  uint32_t size = sizeof(exe);
  if (_NSGetExecutablePath(exe, &size) == 0) {
    char *resolved = realpath(exe, NULL);
    if (resolved) {
      strncpy(exe, resolved, sizeof(exe) - 1);
      free(resolved);
    }
    found = true;
  }
#else
  ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
  if (len > 0) {
    exe[len] = '\0';
    found = true;
  }
#endif

  if (found) {
    char *dir = dirname(exe);
    strncpy(exe_dir, dir, sizeof(exe_dir) - 1);
  } else {
    strncpy(exe_dir, ".", sizeof(exe_dir) - 1);
  }

  // SDK files are in sdk/ subdirectory next to executable
  snprintf(sdk_header_path, sizeof(sdk_header_path), "%s/sdk", exe_dir);
  snprintf(sdk_source_path, sizeof(sdk_source_path), "%s/sdk/led_viz_sdk.c",
           exe_dir);

  // Compiled library goes in temp
  snprintf(compiled_lib_path, sizeof(compiled_lib_path),
           "/tmp/led_viz_user%s", LIB_EXT);
}

static time_t get_mtime(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;
  return st.st_mtime;
}

static bool compile_source(const char *source_path) {
  char cmd[8192];

  // Detect compiler
  const char *cc = getenv("CC");
  if (!cc)
    cc = "cc";

  // Compile user source + SDK source into shared library
  snprintf(cmd, sizeof(cmd),
           "%s -shared -fPIC -O2 -o '%s' '%s' '%s' -I'%s' -lm 2>&1", cc,
           compiled_lib_path, source_path, sdk_source_path, sdk_header_path);

  TraceLog(LOG_INFO, "Compiling: %s", cmd);

  FILE *fp = popen(cmd, "r");
  if (!fp) {
    TraceLog(LOG_ERROR, "Failed to run compiler");
    return false;
  }

  char output[4096] = {0};
  size_t total = 0;
  char buf[256];
  while (fgets(buf, sizeof(buf), fp) && total < sizeof(output) - 1) {
    size_t len = strlen(buf);
    memcpy(output + total, buf, len);
    total += len;
  }
  output[total] = '\0';

  int status = pclose(fp);
  if (status != 0) {
    TraceLog(LOG_ERROR, "Compilation failed:\n%s", output);
    return false;
  }

  if (total > 0) {
    TraceLog(LOG_WARNING, "Compiler output:\n%s", output);
  }

  TraceLog(LOG_INFO, "Compilation successful");
  return true;
}

static LoadedPrograms load_programs(void) {
  LoadedPrograms loaded = {0};

  loaded.handle = dlopen(compiled_lib_path, RTLD_NOW);
  if (!loaded.handle) {
    TraceLog(LOG_ERROR, "dlopen failed: %s", dlerror());
    return loaded;
  }

  loaded.programs = dlsym(loaded.handle, "programs");
  loaded.num_programs = dlsym(loaded.handle, "NUM_PROGRAMS");

  if (!loaded.programs || !loaded.num_programs) {
    TraceLog(LOG_ERROR,
             "Missing symbols. Make sure your file defines:\n"
             "  const Program programs[] = { ... };\n"
             "  const int NUM_PROGRAMS = ...;");
    dlclose(loaded.handle);
    loaded.handle = NULL;
    return loaded;
  }

  TraceLog(LOG_INFO, "Loaded %d program(s)", *loaded.num_programs);
  return loaded;
}

static void unload_programs(LoadedPrograms *loaded) {
  if (loaded->handle) {
    dlclose(loaded->handle);
    loaded->handle = NULL;
    loaded->programs = NULL;
    loaded->num_programs = NULL;
  }
}

static void print_usage(const char *prog) {
  fprintf(stderr, "LED Visualizer - Hot-reloading LED program simulator\n\n");
  fprintf(stderr, "Usage: %s [options] <programs.c>\n\n", prog);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  --strips N    Number of LED strips (1-4, default: 4)\n");
  fprintf(stderr, "  --leds N      LEDs per strip (1-144, default: 144)\n\n");
  fprintf(stderr, "Example:\n");
  fprintf(stderr, "  %s --strips 2 --leds 60 ./programs.c\n\n", prog);
  fprintf(stderr, "The source file should include <led_viz.h> and define:\n");
  fprintf(stderr, "  const Program programs[] = { ... };\n");
  fprintf(stderr, "  const int NUM_PROGRAMS = ...;\n");
}

int main(int argc, char *argv[]) {
  int num_strips = 4;
  int num_leds = 144;
  const char *source_arg = NULL;

  // Parse arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "--strips") == 0 && i + 1 < argc) {
      num_strips = atoi(argv[++i]);
      if (num_strips < 1)
        num_strips = 1;
      if (num_strips > 4)
        num_strips = 4;
    } else if (strcmp(argv[i], "--leds") == 0 && i + 1 < argc) {
      num_leds = atoi(argv[++i]);
      if (num_leds < 1)
        num_leds = 1;
      if (num_leds > 144)
        num_leds = 144;
    } else if (argv[i][0] != '-') {
      source_arg = argv[i];
    }
  }

  if (!source_arg) {
    print_usage(argv[0]);
    return 1;
  }

  // Resolve paths
  resolve_exe_dir();

  // Get absolute path to source file
  char *resolved = realpath(source_arg, NULL);
  if (!resolved) {
    fprintf(stderr, "Error: Cannot find source file: %s\n", source_arg);
    return 1;
  }
  strncpy(source_file_path, resolved, sizeof(source_file_path) - 1);
  free(resolved);

  // Check SDK files exist
  if (access(sdk_source_path, R_OK) != 0) {
    fprintf(stderr, "Error: SDK not found at %s\n", sdk_header_path);
    fprintf(stderr, "Make sure the 'sdk' folder is next to the executable.\n");
    return 1;
  }

  // Initial compilation
  if (!compile_source(source_file_path)) {
    fprintf(stderr, "Initial compilation failed. Fix errors and restart.\n");
    return 1;
  }

  // Initialize window
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(1280, 720, "LED Visualizer");
  SetTargetFPS(60);

  // Load visualizer state
  VisualizerState state = {0};
  visualizer_init(&state, num_strips, num_leds);

  // Load user programs
  LoadedPrograms loaded = load_programs();
  time_t last_mtime = get_mtime(source_file_path);

  while (!WindowShouldClose()) {
    // Check for source file changes
    time_t current_mtime = get_mtime(source_file_path);
    if (current_mtime != last_mtime) {
      TraceLog(LOG_INFO, "Source file changed, recompiling...");

      // Small delay for editor to finish writing
      usleep(100000);

      if (compile_source(source_file_path)) {
        unload_programs(&loaded);
        loaded = load_programs();

        // Reset to first program if current is out of range
        if (loaded.num_programs &&
            state.active_program >= *loaded.num_programs) {
          state.active_program = 0;
        }
      }

      last_mtime = get_mtime(source_file_path);
    }

    // Update programs in state from loaded programs
    if (loaded.programs && loaded.num_programs && *loaded.num_programs > 0) {
      state.programs = loaded.programs;
      state.num_programs = *loaded.num_programs;

      // Clamp program index
      if (state.active_program >= state.num_programs) {
        state.active_program = 0;
      }
      state.current_program = &state.programs[state.active_program];
    } else {
      state.programs = NULL;
      state.num_programs = 0;
      state.current_program = NULL;
    }

    visualizer_update(&state);
    visualizer_draw(&state);
  }

  unload_programs(&loaded);
  CloseWindow();
  return 0;
}
