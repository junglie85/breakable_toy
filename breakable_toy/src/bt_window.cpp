#include "bt_window.hpp"

#include "bt_logger.hpp"

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

    auto glad_vk_version = gladLoaderLoadVulkan(nullptr, nullptr, nullptr);
    if (!glad_vk_version) {
        throw std::runtime_error("unable to load Vulkan symbols, gladLoad failure");
    }

    auto major = GLAD_VERSION_MAJOR(glad_vk_version);
    auto minor = GLAD_VERSION_MINOR(glad_vk_version);
    SPDLOG_INFO("Vulkan version {}.{}", major, minor);
}
} // namespace bt
