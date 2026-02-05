# ESP-IDF Component CMakeLists.txt
# Put this directory in your project's components/ folder

idf_component_register(
    SRCS
        "led_viz_esp32.c"
        "led_viz_sdk.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        led_strip
        esp_timer
)
