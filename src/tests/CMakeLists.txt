add_library(unity unity/unity.c)

add_executable(test_visualizer test_visualizer.c)
target_link_libraries(test_virualizer PRIVATE visualizer unity)

add_test(NAME test_visualizer COMMAND test_visualizer)
