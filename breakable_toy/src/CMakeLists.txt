add_executable(toy main.cpp)

target_link_libraries(toy PRIVATE fmt::fmt glfw spdlog::spdlog)
