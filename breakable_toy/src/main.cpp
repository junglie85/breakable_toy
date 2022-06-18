#include <glad/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <stdexcept>
#include <vector>

#ifdef NDEBUG
const bool vk_debug = false;
#else
const bool vk_debug = true;
#endif

namespace bt {
class logger {
  public:
    static logger init()
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        std::array<spdlog::sink_ptr, 1> sinks { console_sink };

        auto internal = std::make_shared<spdlog::logger>("BT", sinks.begin(), sinks.end());
        internal->set_pattern("[%^%l%$] %v");
        internal->set_level(spdlog::level::trace);

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
            throw std::runtime_error("unable to initialise GLFW");
        }

        if (!glfwVulkanSupported()) {
            throw std::runtime_error("no Vulkan loader or installable client driver found");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        auto native = glfwCreateWindow(1280, 720, "Breakable Toy", nullptr, nullptr);
        if (!native) {
            throw std::runtime_error("unable to create GLFW window");
        }

        return window(native);
    }

    window(const window&) = delete;
    window(window&&) = delete;
    ~window() = default;

    void begin_frame() { glfwPollEvents(); }

    bool close_requested() const { return should_close; }

    void destroy()
    {
        glfwDestroyWindow(handle);
        glfwTerminate();
    }

    void end_frame() { should_close = glfwWindowShouldClose(handle); }

    GLFWwindow* get_handle() const { return handle; }

  private:
    window(GLFWwindow* native) : handle(native) { }

    GLFWwindow* handle;
    bool should_close;
};

#define VK_THROW(result, ...)                                                                      \
    if ((result) != VK_SUCCESS) {                                                                  \
        throw std::runtime_error(fmt::format(__VA_ARGS__));                                        \
    }

const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* pUsuser_dataerData)
{
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        SPDLOG_ERROR("{} - {} - {}", callback_data->messageIdNumber, callback_data->pMessageIdName,
            callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        SPDLOG_WARN("{} - {} - {}", callback_data->messageIdNumber, callback_data->pMessageIdName,
            callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        SPDLOG_INFO("{} - {} - {}", callback_data->messageIdNumber, callback_data->pMessageIdName,
            callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        SPDLOG_TRACE("{} - {} - {}", callback_data->messageIdNumber, callback_data->pMessageIdName,
            callback_data->pMessage);
    }

    return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* debug_utils_create_info,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_utils_messenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (fn != nullptr) {
        return fn(instance, debug_utils_create_info, allocator, debug_utils_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_ext(VkInstance instance,
    VkDebugUtilsMessengerEXT debug_utils_messenger,
    const VkAllocationCallbacks* allocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (fn != nullptr) {
        fn(instance, debug_utils_messenger, allocator);
    }
}

class gpu {
  public:
    static gpu init(const window& w)
    {
        VkAllocationCallbacks* allocator = nullptr;

        gpu g(allocator);

        g.init_glad();
        g.create_instance();
        g.create_debug_utils_messenger();
        g.create_surface(w);

        // g.init_glad(); // Do this again once we have an instance, physical device and device.
        // https://github.com/Dav1dde/glad/blob/glad2/example/c/vulkan_tri_glfw/vulkan_tri_glfw.c#L1837-L1841

        return g;
    }

    gpu(const gpu&) = delete;
    ~gpu() = default;

    void destroy()
    {
        vkDestroySurfaceKHR(instance, surface, allocator);
        if (vk_debug) {
            destroy_debug_utils_messenger_ext(instance, debug_utils_messenger, allocator);
        }
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
            throw std::runtime_error("unable to load Vulkan symbols, gladLoad failure");
        }

        auto major = GLAD_VERSION_MAJOR(glad_vk_version);
        auto minor = GLAD_VERSION_MINOR(glad_vk_version);
        SPDLOG_INFO("Vulkan version {}.{}", major, minor);
    }

    void create_instance()
    {
        auto required_layers = get_required_validation_layers();
        auto available_layers = get_available_validation_layers();
        if (vk_debug && !check_validation_layer_support(required_layers, available_layers)) {
            throw std::runtime_error("not all required Vulkan validation layers were available");
        } else if (vk_debug) {
            for (auto& layer : required_layers) {
                SPDLOG_DEBUG("enabled Vulkan validation layer {}", layer);
            }
        }

        auto required_extensions = get_required_extensions();
        auto available_extensions = get_available_extensions();
        if (!check_extension_support(required_extensions, available_extensions)) {
            throw std::runtime_error("not all required Vulkan extensions were available");
        } else {
            for (auto& extension : required_extensions) {
                SPDLOG_DEBUG("enabled Vulkan extension {}", extension);
            }
        }

        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pNext = nullptr;
        app_info.pApplicationName = "Breakable Toy";
        app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.pEngineName = "Breakable Toy";
        app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        instance_info.pNext = nullptr;
        instance_info.flags = VK_VALIDATION_CHECK_ALL_EXT;
        instance_info.pApplicationInfo = &app_info;

        if (vk_debug) {
            instance_info.enabledLayerCount = static_cast<uint32_t>(required_layers.size());
            instance_info.ppEnabledLayerNames = required_layers.data();

            auto debug_utils_create_info = get_debug_utils_create_info();
            instance_info.pNext = &debug_utils_create_info;
        } else {
            instance_info.enabledLayerCount = 0;
            instance_info.ppEnabledLayerNames = nullptr;
        }

        instance_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
        instance_info.ppEnabledExtensionNames = required_extensions.data();

        VK_THROW(vkCreateInstance(&instance_info, allocator, &instance),
            "unable to create Vulkan instance");
    }

    std::vector<const char*> get_required_validation_layers()
    {
        const std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

        for (auto& layer : layers) {
            SPDLOG_TRACE("required Vulkan validation layer {}", layer);
        }

        return layers;
    }

    std::vector<VkLayerProperties> get_available_validation_layers()
    {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

        for (auto& layer : layers) {
            SPDLOG_TRACE("available Vulkan validation layer {}", layer.layerName);
        }

        return layers;
    }

    bool check_validation_layer_support(
        const std::vector<const char*>& required, const std::vector<VkLayerProperties>& available)
    {
        for (auto& layer : required) {
            bool found = std::any_of(available.begin(), available.end(),
                [layer](auto& a) { return strcmp(a.layerName, layer) == 0; });

            if (!found) {
                SPDLOG_TRACE("required Vulkan validation layer {} but was not available", layer);
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> get_required_extensions()
    {
        uint32_t required_extension_count = 0;
        auto required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

        std::vector<const char*> extensions(
            required_extensions, required_extensions + required_extension_count);

        if (vk_debug) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        for (auto& extension : extensions) {
            SPDLOG_TRACE("required Vulkan extension {}", extension);
        }

        return extensions;
    }

    std::vector<VkExtensionProperties> get_available_extensions()
    {
        uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

        for (auto& extension : extensions) {
            SPDLOG_TRACE("available Vulkan extension {}", extension.extensionName);
        }

        return extensions;
    }

    bool check_extension_support(const std::vector<const char*>& required,
        const std::vector<VkExtensionProperties>& available)
    {
        for (auto& extension : required) {
            bool found = std::any_of(available.begin(), available.end(),
                [extension](auto& a) { return strcmp(a.extensionName, extension) == 0; });

            if (!found) {
                SPDLOG_TRACE("required Vulkan extension {} but was not available", extension);
                return false;
            }
        }

        return true;
    }

    void create_debug_utils_messenger()
    {
        if (!vk_debug) {
            return;
        }

        auto debug_utils_create_info = get_debug_utils_create_info();
        VK_THROW(create_debug_utils_messenger_ext(
                     instance, &debug_utils_create_info, nullptr, &debug_utils_messenger),
            "unable to create debug utils messenger");
    }

    VkDebugUtilsMessengerCreateInfoEXT get_debug_utils_create_info()
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_utils_create_info {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
        };
        debug_utils_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_utils_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_utils_create_info.pfnUserCallback = debug_messenger_callback;
        debug_utils_create_info.pUserData = nullptr;

        return debug_utils_create_info;
    }

    void create_surface(const window& w)
    {
        VK_THROW(glfwCreateWindowSurface(instance, w.get_handle(), allocator, &surface),
            "unable to create Vulkan window surface");
    }

    VkAllocationCallbacks* allocator;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_utils_messenger;
    VkSurfaceKHR surface;
};
} // namespace bt

int main(int argc, char* argv[])
{
    try {
        auto logger = bt::logger::init();
        auto window = bt::window::init();
        auto gpu = bt::gpu::init(window);

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
    } catch (std::exception e) {
        SPDLOG_CRITICAL(e.what());

        return EXIT_FAILURE;
    }
}
