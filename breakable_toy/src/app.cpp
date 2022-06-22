#include "app.hpp"

#include "bt_logger.hpp"

#include <array>
#include <stdexcept>

namespace bt {
app::app()
{
    create_pipeline_layout();
    create_pipeline();
    create_command_buffers();
}

app::~app() { vkDestroyPipelineLayout(device.device(), pipeline_layout, device.allocator()); }

void app::run()
{
    while (!window.should_close()) {
        glfwPollEvents();
        draw_frame();
    }

    vkDeviceWaitIdle(device.device());
}

void app::create_pipeline_layout()
{
    VkPipelineLayoutCreateInfo pipeline_layout_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pSetLayouts = nullptr;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device.device(), &pipeline_layout_info, device.allocator(), &pipeline_layout)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void app::create_pipeline()
{
    auto pipeline_config = bt_pipeline::default_pipeline_config_info(swapchain.width(), swapchain.height());
    pipeline_config.render_pass = swapchain.render_pass();
    pipeline_config.pipeline_layout = pipeline_layout;

    pipeline = std::make_unique<bt_pipeline>(device,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipeline_config);
}

void app::create_command_buffers()
{
    command_buffers.resize(swapchain.image_count());

    VkCommandBufferAllocateInfo alloc_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = device.command_pool();
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

    if (vkAllocateCommandBuffers(device.device(), &alloc_info, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for (int i = 0; i < command_buffers.size(); ++i) {
        VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

        if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo render_pass_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        render_pass_info.renderPass = swapchain.render_pass();
        render_pass_info.framebuffer = swapchain.framebuffer(i);
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = swapchain.swapchain_extent();

        std::array<VkClearValue, 2> clear_values {};
        clear_values[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
        clear_values[1].depthStencil = { 1.0f, 0 };
        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        pipeline->bind(command_buffers[i]);
        vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffers[i]);

        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
    }
}

void app::draw_frame()
{
    uint32_t image_index;
    auto result = swapchain.acquire_next_image(&image_index);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    result = swapchain.submit_command_buffers(&command_buffers[image_index], &image_index);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swapchain image");
    }
}
} // namespace bt
