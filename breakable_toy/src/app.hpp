#ifndef APP_HPP
#define APP_HPP

#include "bt_device.hpp"
#include "bt_pipeline.hpp"
#include "bt_window.hpp"

namespace bt {
class app {
  public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 720;

    void run();

  private:
    bt_window window { WIDTH, HEIGHT, "Breakable Toy" };
    bt_device device { window };
    bt_pipeline pipeline { device, "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        bt_pipeline::default_pipeline_config_info(WIDTH, HEIGHT) };
};
} // namespace bt

#endif // APP_HPP
