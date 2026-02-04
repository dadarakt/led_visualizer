cmake_minimum_required(VERSION 3.22)
project(ray_visualizer C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/extern/raylib/raylib")
set(BUILD_SHARED_LIBS ON)
add_subdirectory(extern/raylib)

add_library(visualizer SHARED
    src/visualizer.c
)
target_link_libraries(visualizer PRIVATE raylib)

add_executable(bullet_hell
    src/main.c
)
target_link_libraries(bullet_hell PRIVATE raylib dl)

# Platform extras
if (APPLE)
    set_target_properties(visualizer PROPERTIES SUFFIX ".dylib")
    target_link_libraries(bullet_hell PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
    )
    target_link_options(bullet_hell PRIVATE "-Wl,-rpath,@loader_path/../extern/raylib")
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(bullet_hell PRIVATE m pthread)
    target_link_options(bullet_hell PRIVATE "-Wl,-rpath,\$ORIGIN/extern/raylib/raylib")
endif()



