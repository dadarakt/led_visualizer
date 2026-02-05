cmake_minimum_required(VERSION 3.22)
project(led_visualizer C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(BUILD_SHARED_LIBS ON)
add_subdirectory(extern/raylib)

# Main executable - includes visualizer code directly
add_executable(led_viz
    src/main.c
    src/visualizer.c
    src/palette.c
)
target_link_libraries(led_viz PRIVATE raylib dl)
target_include_directories(led_viz PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Copy shader resources to build directory
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
  add_custom_command(TARGET led_viz POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources)
endif()

# Copy SDK files to build directory (for runtime compilation)
add_custom_command(TARGET led_viz POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/sdk
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/include/led_viz.h
        ${CMAKE_SOURCE_DIR}/include/led_viz_sdk.c
        ${CMAKE_BINARY_DIR}/sdk/
    DEPENDS
        ${CMAKE_SOURCE_DIR}/include/led_viz.h
        ${CMAKE_SOURCE_DIR}/include/led_viz_sdk.c
)

# Platform extras
if (APPLE)
  target_link_libraries(led_viz PRIVATE
      "-framework Cocoa"
      "-framework OpenGL"
      "-framework IOKit"
  )
  target_link_options(led_viz PRIVATE
    "-Wl,-rpath,@loader_path/extern/raylib/raylib"
  )
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(led_viz PRIVATE m pthread)
  target_link_options(led_viz PRIVATE
    "-Wl,-rpath,\$ORIGIN/extern/raylib/raylib"
  )
endif()

# Install targets
install(TARGETS led_viz RUNTIME DESTINATION bin)
install(FILES
    ${CMAKE_SOURCE_DIR}/include/led_viz.h
    ${CMAKE_SOURCE_DIR}/include/led_viz_sdk.c
    DESTINATION share/led_viz/sdk
)
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION share/led_viz)
endif()
