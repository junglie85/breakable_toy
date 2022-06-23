#include "app.hpp"

#include "bt_logger.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace bt {
app::app()
{
    load_models();
    create_pipeline_layout();
    recreate_swapchain();
    create_command_buffers();
}

app::~app() { vkDestroyPipelineLayout(device.device(), pipeline_layout, device.allocator()); }

void app::run()
{
    while (!window.should_close()) {
        glfwPollEvents();
        draw_frame();
        // vkDeviceWaitIdle(device.device());
    }

    vkDeviceWaitIdle(device.device());
}

void app::load_models()
{
    std::vector<bt_model::vertex> vertices {
        // clang-format off
        {{ 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        {{ 0.5f, 0.5f },  { 0.0f, 1.0f, 0.0f }},
        {{ -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }}
        // clang-format on
    };
    model = std::make_unique<bt_model>(device, vertices);
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
    assert(swapchain != nullptr && "cannot create pipeline before swapchain");
    assert(pipeline_layout != nullptr && "cannot create pipeline before pipeline layout");

    bt_pipeline_config_info pipeline_config {};
    bt_pipeline::default_pipeline_config_info(pipeline_config);
    pipeline_config.render_pass = swapchain->render_pass();
    pipeline_config.pipeline_layout = pipeline_layout;

    pipeline = std::make_unique<bt_pipeline>(device,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipeline_config);
}

void app::create_command_buffers()
{
    command_buffers.resize(swapchain->image_count());

    VkCommandBufferAllocateInfo alloc_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = device.command_pool();
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

    if (vkAllocateCommandBuffers(device.device(), &alloc_info, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }
}

void app::free_command_buffers()
{
    vkFreeCommandBuffers(device.device(),
        device.command_pool(),
        static_cast<uint32_t>(command_buffers.size()),
        command_buffers.data());
    command_buffers.clear();
}

void app::draw_frame()
{
    uint32_t image_index;
    auto result = swapchain->acquire_next_image(&image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image");
    }

    record_command_buffer(image_index);
    result = swapchain->submit_command_buffers(&command_buffers[image_index], &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.was_resized()) {
        window.reset_resized_flag();
        recreate_swapchain();
        return;
    }

    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swapchain image");
    }
}

void app::recreate_swapchain()
{
    auto extent = window.extent();
    while (extent.width == 0 || extent.height == 0) {
        extent = window.extent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device.device());

    if (swapchain == nullptr) {
        swapchain = std::make_unique<bt_swapchain>(device, extent);
    } else {
        swapchain = std::make_unique<bt_swapchain>(device, extent, std::move(swapchain));
        if (swapchain->image_count() != command_buffers.size()) {
            free_command_buffers();
            create_command_buffers();
        }
    }

    // TODO: check if render pass is compatible and only create pipeline if it is not.
    create_pipeline();
}

void app::record_command_buffer(uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

    if (vkBeginCommandBuffer(command_buffers[image_index], &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo render_pass_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    render_pass_info.renderPass = swapchain->render_pass();
    render_pass_info.framebuffer = swapchain->framebuffer(image_index);
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = swapchain->swapchain_extent();

    std::array<VkClearValue, 2> clear_values {};
    clear_values[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
    clear_values[1].depthStencil = { 1.0f, 0 };
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffers[image_index], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(swapchain->swapchain_extent().width);
    viewport.height = static_cast<float>(swapchain->swapchain_extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor { { 0, 0 }, swapchain->swapchain_extent() };
    vkCmdSetViewport(command_buffers[image_index], 0, 1, &viewport);
    vkCmdSetScissor(command_buffers[image_index], 0, 1, &scissor);

    pipeline->bind(command_buffers[image_index]);
    model->bind(command_buffers[image_index]);
    model->draw(command_buffers[image_index]);

    vkCmdEndRenderPass(command_buffers[image_index]);

    if (vkEndCommandBuffer(command_buffers[image_index]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}
} // namespace bt
