# LED Visualizer - ESP32 Runtime

Thin runtime layer to run your LED programs on ESP32 hardware using ESP-IDF's `led_strip` component.

## Usage

### Option A: PlatformIO (ESP-IDF framework)

**platformio.ini:**

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = espidf

lib_deps =
    espressif/led_strip@^2.0.0
```

**Project structure:**

```
your_project/
├── platformio.ini
├── components/
│   └── led_viz/           # Copy this esp32/ directory here
│       ├── CMakeLists.txt
│       ├── led_viz.h
│       ├── led_viz_sdk.c
│       ├── led_viz_esp32.h
│       └── led_viz_esp32.c
├── src/
│   └── main.c
└── shared/
    └── programs.c         # Your effects (shared with visualizer)
```

### Option B: Native ESP-IDF

Copy this entire `esp32/` directory into your project's `components/` folder:

```
your_project/
├── main/
│   ├── CMakeLists.txt
│   ├── main.c
│   └── programs.c      # Your LED programs (same file used in visualizer!)
└── components/
    └── led_viz/        # This directory
        ├── CMakeLists.txt
        ├── led_viz.h
        ├── led_viz_sdk.c
        ├── led_viz_esp32.h
        └── led_viz_esp32.c
```

**Add led_strip dependency** in your project's `main/idf_component.yml`:

```yaml
dependencies:
  espressif/led_strip: "^2.0.0"
```

Or add via: `idf.py add-dependency "espressif/led_strip^2.0.0"`

### Write your main.c

```c
#include "led_viz_esp32.h"

// Include your programs
#include "programs.c"

void app_main(void) {
    LedVizConfig config = {
        .strips = {
            {.gpio = 18, .num_leds = 144},
            // ... more strips
        },
        .num_strips = 1,
        .target_fps = 60,
    };

    led_viz_init(&config);
    led_viz_set_program(0);
    led_viz_set_palette(&PALETTE_RAINBOW);
    led_viz_run();  // Blocking
}
```

### Share programs between visualizer and ESP32

The key benefit: **same `programs.c` runs on both desktop visualizer and ESP32**.

```
your_project/
├── shared/
│   └── programs.c       # Your effects
├── visualizer/          # Desktop - uses led_viz tool
└── firmware/            # ESP32 - uses this runtime
    └── main/
        └── main.c       # #include "../shared/programs.c"
```

## API

```c
// Initialize with strip configuration
int led_viz_init(const LedVizConfig *config);

// Select active program (index into programs[])
void led_viz_set_program(int index);

// Select active palette
void led_viz_set_palette(const Palette16 *palette);

// Run animation loop (blocking)
void led_viz_run(void);

// Stop animation loop
void led_viz_stop(void);

// Cleanup
void led_viz_deinit(void);
```

## Notes

- Uses RMT peripheral for precise WS2812B timing
- Frame timing via `esp_timer` for consistent FPS
- Same `PixelFunc` interface as visualizer
- Programs are completely portable between platforms
