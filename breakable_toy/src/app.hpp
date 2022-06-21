#ifndef APP_HPP
#define APP_HPP

#include "bt_device.hpp"
#include "bt_pipeline.hpp"
#include "bt_swapchain.hpp"
#include "bt_window.hpp"

#include <memory>
#include <vector>

namespace bt {
class app {
  public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 720;

    app();
    app(const app&) = delete;
    ~app();

    app& operator=(const app&) = delete;

    void run();

  private:
    void create_pipeline_layout();
    void create_pipeline();
    void create_command_buffers();
    void draw_frame();

    bt_window window { WIDTH, HEIGHT, "Breakable Toy" };
    bt_device device { window };
    bt_swapchain swapchain { device, window.extent() };
    std::unique_ptr<bt_pipeline> pipeline;
    VkPipelineLayout pipeline_layout;
    std::vector<VkCommandBuffer> command_buffers;
};
} // namespace bt

#endif // APP_HPP
