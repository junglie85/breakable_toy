#ifndef APP_HPP
#define APP_HPP

#include "bt_device.hpp"
#include "bt_model.hpp"
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
    void load_models();
    void create_pipeline_layout();
    void create_pipeline();
    void create_command_buffers();
    void free_command_buffers();
    void draw_frame();
    void recreate_swapchain();
    void record_command_buffer(uint32_t image_index);

    bt_window window { WIDTH, HEIGHT, "Breakable Toy" };
    bt_device device { window };
    std::unique_ptr<bt_swapchain> swapchain;
    std::unique_ptr<bt_pipeline> pipeline;
    VkPipelineLayout pipeline_layout;
    std::vector<VkCommandBuffer> command_buffers;
    std::unique_ptr<bt_model> model;
};
} // namespace bt

#endif // APP_HPP
