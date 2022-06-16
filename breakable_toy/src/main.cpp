#include <glad/vulkan.h>
#define GLFW_INCLUDE_NONE
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

    auto glad_vk_version = gladLoaderLoadVulkan(nullptr, nullptr, nullptr);
    if (!glad_vk_version) {
        SPDLOG_CRITICAL("unable to load Vulkan symbols, gladLoad failure");
        return EXIT_FAILURE;
    }

    auto major = GLAD_VERSION_MAJOR(glad_vk_version);
    auto minor = GLAD_VERSION_MINOR(glad_vk_version);
    SPDLOG_INFO("Vulkan version {}.{}", major, minor);

    glfwSetErrorCallback([](int code, const char* description) {
        SPDLOG_ERROR("GLFW error (code {}): {}", code, description);
    });

    glfwInitVulkanLoader(vkGetInstanceProcAddr);

    if (!glfwInit()) {
        SPDLOG_CRITICAL("unable to initialise GLFW");
        return EXIT_FAILURE;
    }

    if (!glfwVulkanSupported()) {
        SPDLOG_CRITICAL("no Vulkan loader or installable client driver found");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(1280, 720, "Breakable Toy", nullptr, nullptr);
    if (!window) {
        SPDLOG_CRITICAL("unable to create GLFW window");
        return EXIT_FAILURE;
    }

    VkAllocationCallbacks* vkAllocator = nullptr;

    // TODO: Create a basic instance to check we have loaded Vulkan correctly.
    auto create_info = VkInstanceCreateInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance instance {};
    VkResult result = vkCreateInstance(&create_info, vkAllocator, &instance);
    if (result != VK_SUCCESS) {
        SPDLOG_CRITICAL("unable to create Vulkan instance");
        return EXIT_FAILURE;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkDestroyInstance(instance, vkAllocator);

    glfwDestroyWindow(window);
    glfwTerminate();

    gladLoaderUnloadVulkan();

    return EXIT_SUCCESS;
}
