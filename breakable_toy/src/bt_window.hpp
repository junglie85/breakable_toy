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

  private:
    void init_window();

    const uint32_t width;
    const uint32_t height;
    std::string window_name;
    GLFWwindow* handle;
};
} // namespace bt

#endif // BT_WINDOW_HPP
