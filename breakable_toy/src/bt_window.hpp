#ifndef BT_WINDOW_HPP
#define BT_WINDOW_HPP

#include <glad/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>

namespace bt {
class bt_window {
  public:
    bt_window(uint32_t width, uint32_t height, std::string name);
    bt_window(const bt_window&) = delete;
    bt_window(bt_window&&) = delete;
    ~bt_window();

    bt_window& operator=(const bt_window&) = delete;

    bool should_close() { return glfwWindowShouldClose(handle); }
    VkExtent2D extent() { return { width, height }; }
    bool was_resized() { return framebuffer_resized; }
    void reset_resized_flag() { framebuffer_resized = false; }

    void create_window_surface(VkInstance instance, VkSurfaceKHR* surface, VkAllocationCallbacks* allocator);

  private:
    void init_window();

    uint32_t width;
    uint32_t height;
    bool framebuffer_resized = false;
    std::string window_name;
    GLFWwindow* handle;
};
} // namespace bt

#endif // BT_WINDOW_HPP
