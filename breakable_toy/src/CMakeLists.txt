add_executable(toy main.cpp)

include(FindVulkan)

target_link_libraries(toy PRIVATE fmt::fmt glad_vulkan_12 glfw spdlog::spdlog)
