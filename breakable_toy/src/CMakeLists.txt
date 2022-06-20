add_executable(toy main.cpp)

target_link_libraries(toy PRIVATE fmt::fmt glad_vulkan_12 glfw glm spdlog::spdlog)
