#include <glad/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <array>
#include <format>
#include <iostream>

namespace bt {
class logger {
  public:
    static logger init()
    {

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        std::array<spdlog::sink_ptr, 1> sinks { console_sink };

        auto internal = std::make_shared<spdlog::logger>("BT", sinks.begin(), sinks.end());
        internal->set_pattern("[%^%l%$] %v");
        internal->set_level(spdlog::level::info);

        spdlog::set_default_logger(internal);

        return logger(internal);
    }

    logger(const logger&) = delete;
    logger(logger&&) = delete;
    ~logger() = default;

    void destroy() { internal->flush(); }

  private:
    logger(std::shared_ptr<spdlog::logger> internal) : internal(internal) { }

    std::shared_ptr<spdlog::logger> internal;
};

class window {
  public:
    static window init()
    {
        glfwSetErrorCallback([](int code, const char* description) {
            SPDLOG_ERROR("GLFW error (code {}): {}", code, description);
        });

        glfwInitVulkanLoader(vkGetInstanceProcAddr);

        if (!glfwInit()) {
            SPDLOG_CRITICAL("unable to initialise GLFW");
        }

        if (!glfwVulkanSupported()) {
            SPDLOG_CRITICAL("no Vulkan loader or installable client driver found");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        auto native = glfwCreateWindow(1280, 720, "Breakable Toy", nullptr, nullptr);
        if (!native) {
            SPDLOG_CRITICAL("unable to create GLFW window");
        }

        return window(native);
    }

    window(const window&) = delete;
    window(window&&) = delete;
    ~window() = default;

    void begin_frame() { glfwPollEvents(); }

    bool close_requested() const { return should_close; }

    void end_frame() { should_close = glfwWindowShouldClose(native); }

    void destroy()
    {
        glfwDestroyWindow(native);
        glfwTerminate();
    }

  private:
    window(GLFWwindow* native) : native(native) { }

    GLFWwindow* native;
    bool should_close;
};

class gpu {
  public:
    static gpu init()
    {
        VkAllocationCallbacks* allocator = nullptr;

        gpu g(allocator);

        g.init_glad();
        g.create_instance();

        return g;
    }

    gpu(const gpu&) = delete;
    ~gpu() = default;

    void destroy()
    {
        vkDestroyInstance(instance, allocator);
        gladLoaderUnloadVulkan();
    }

  private:
    gpu(VkAllocationCallbacks* allocator) : allocator(allocator) { }
    gpu(gpu&&) = default;

    void init_glad()
    {
        auto glad_vk_version = gladLoaderLoadVulkan(nullptr, nullptr, nullptr);
        if (!glad_vk_version) {
            SPDLOG_CRITICAL("unable to load Vulkan symbols, gladLoad failure");
        }

        auto major = GLAD_VERSION_MAJOR(glad_vk_version);
        auto minor = GLAD_VERSION_MINOR(glad_vk_version);
        SPDLOG_INFO("Vulkan version {}.{}", major, minor);
    }

    void create_instance()
    {
        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pNext = nullptr;
        app_info.pApplicationName = "Breakable Toy";
        app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.pEngineName = "Breakable Toy";
        app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo create_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        create_info.pNext = nullptr;
        create_info.flags = VK_VALIDATION_CHECK_ALL_EXT;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledLayerCount = 0;
        create_info.ppEnabledLayerNames = nullptr;
        create_info.enabledExtensionCount = 0;
        create_info.ppEnabledExtensionNames = nullptr;

        auto result = vkCreateInstance(&create_info, allocator, &instance);
        if (result != VK_SUCCESS) {
            SPDLOG_CRITICAL("unable to create Vulkan instance");
        }
    }

    VkAllocationCallbacks* allocator;
    VkInstance instance;
};
} // namespace bt

int main(int argc, char* argv[])
{
    auto logger = bt::logger::init();
    auto window = bt::window::init();
    auto gpu = bt::gpu::init();

    bool running = true;
    while (running) {
        window.begin_frame();
        window.end_frame();

        running = !window.close_requested();
    }

    gpu.destroy();
    window.destroy();
    logger.destroy();

    return EXIT_SUCCESS;
}
