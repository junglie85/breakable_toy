#include <GLFW/glfw3.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <array>
#include <format>
#include <iostream>

int main(int argc, char* argv[])
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::array<spdlog::sink_ptr, 1> sinks { console_sink };

    auto logger = std::make_shared<spdlog::logger>("BT", sinks.begin(), sinks.end());
    logger->set_pattern("[%^%l%$] %v");
    logger->set_level(spdlog::level::info);

    spdlog::set_default_logger(logger);

    SPDLOG_INFO("Welcome to {}!", "Breakable Toy");
    SPDLOG_DEBUG("Should not show");

    glfwSetErrorCallback([](int code, const char* description) {
        SPDLOG_ERROR("GLFW error (code {}): {}", code, description);
    });

    if (!glfwInit()) {
        SPDLOG_CRITICAL("unable to initialise GLFW");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(1280, 720, "Breakable Toy", nullptr, nullptr);
    if (!window) {
        SPDLOG_CRITICAL("unable to create GLFW window");
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
