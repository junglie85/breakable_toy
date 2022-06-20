#include <glad/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <glm/glm.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <array>
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
} // namespace bt

int main(int argc, char* argv[])
{
    auto logger = bt::logger::init();

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
    auto window = glfwCreateWindow(1280, 720, "Breakable Toy", nullptr, nullptr);
    if (!window) {
        throw std::runtime_error("unable to create GLFW window");
    }

    auto glad_vk_version = gladLoaderLoadVulkan(nullptr, nullptr, nullptr);
    if (!glad_vk_version) {
        throw std::runtime_error("unable to load Vulkan symbols, gladLoad failure");
    }

    auto major = GLAD_VERSION_MAJOR(glad_vk_version);
    auto minor = GLAD_VERSION_MINOR(glad_vk_version);
    SPDLOG_INFO("Vulkan version {}.{}", major, minor);

    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    SPDLOG_INFO("{} extensions supported", count);

    glm::mat4x4 test {};
    SPDLOG_INFO("Mat 4[0][0] value: {}", test[0][0]);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    gladLoaderUnloadVulkan();

    glfwDestroyWindow(window);
    glfwTerminate();

    logger.destroy();

    return EXIT_SUCCESS;
}
