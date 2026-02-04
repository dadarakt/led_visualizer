cmake_minimum_required(VERSION 3.22)
project(ray_bullet_hell C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(extern/raylib)

add_executable(bullet_hell
    src/main.c
)

target_link_libraries(bullet_hell PRIVATE raylib)

# Platform niceties
if (APPLE)
    target_link_libraries(bullet_hell PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(bullet_hell PRIVATE m dl pthread)
endif()

