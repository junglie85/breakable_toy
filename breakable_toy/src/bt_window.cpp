#include "bt_window.hpp"

#include "bt_logger.hpp"

#include <stdexcept>

namespace bt {
bt_window::bt_window(uint32_t width, uint32_t height, std::string name) :
    width { width }, height { height }, window_name { std::move(name) }
{
    init_window();
}

bt_window::~bt_window()
{
    gladLoaderUnloadVulkan();

    glfwDestroyWindow(handle);
    glfwTerminate();
}

void bt_window::init_window()
{
    glfwSetErrorCallback([](int code, const char* description) {
        SPDLOG_ERROR("GLFW error (code {}): {}", code, description);
    });

    glfwInitVulkanLoader(vkGetInstanceProcAddr);

    if (!glfwInit()) {
        throw std::runtime_error("unable to initialise GLFW");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("no Vulkan loader or installable client driver found");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    handle = glfwCreateWindow(width, height, window_name.c_str(), nullptr, nullptr);
    if (!handle) {
        throw std::runtime_error("unable to create GLFW window");
    }
}

void bt_window::create_window_surface(
    VkInstance instance, VkSurfaceKHR* surface, VkAllocationCallbacks* allocator)
{
    if (glfwCreateWindowSurface(instance, handle, allocator, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
}
} // namespace bt
