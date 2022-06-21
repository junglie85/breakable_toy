add_executable(toy app.cpp bt_device.cpp bt_filesystem.cpp bt_logger.cpp bt_pipeline.cpp bt_window.cpp main.cpp)

target_link_libraries(toy PRIVATE fmt::fmt glad_vulkan_12 glfw glm spdlog::spdlog)

add_custom_command(TARGET toy POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/src/shaders)
