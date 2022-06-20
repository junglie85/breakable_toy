add_executable(toy app.cpp bt_logger.cpp bt_window.cpp main.cpp)

target_link_libraries(toy PRIVATE fmt::fmt glad_vulkan_12 glfw glm spdlog::spdlog)
