#include "app.hpp"

#include <stdexcept>

namespace bt {
app::app()
{
    create_pipeline_layout();
    create_pipeline();
    // create_command_buffers();
}

app::~app() { vkDestroyPipelineLayout(device.device(), pipeline_layout, device.allocator()); }

void app::run()
{
    while (!window.should_close()) {
        glfwPollEvents();
    }
}

void app::create_pipeline_layout()
{
    VkPipelineLayoutCreateInfo pipeline_layout_info {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(
            device.device(), &pipeline_layout_info, device.allocator(), &pipeline_layout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void app::create_pipeline()
{
    auto pipeline_config
        = bt_pipeline::default_pipeline_config_info(swapchain.width(), swapchain.height());
    pipeline_config.render_pass = swapchain.render_pass();
    pipeline_config.pipeline_layout = pipeline_layout;

    pipeline = std::make_unique<bt_pipeline>(device, "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv", pipeline_config);
}

void app::create_command_buffers() { }

void app::draw_frame() { }
} // namespace bt
