#ifndef BT_PIPELINE_HPP
#define BT_PIPELINE_HPP

#include "bt_device.hpp"

#include <string_view>
#include <vector>

namespace bt {
struct bt_pipeline_config_info {
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineViewportStateCreateInfo viewport_info;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info;
    VkPipelineRasterizationStateCreateInfo rasterisation_info;
    VkPipelineMultisampleStateCreateInfo multisample_info;
    VkPipelineColorBlendAttachmentState color_blend_attachment;
    VkPipelineColorBlendStateCreateInfo color_blend_info;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info;
    VkPipelineLayout pipeline_layout = nullptr;
    VkRenderPass render_pass = nullptr;
    uint32_t subpass = 0;
};

class bt_pipeline {
  public:
    static bt_pipeline_config_info default_pipeline_config_info(uint32_t width, uint32_t height);

    bt_pipeline(bt_device& device,
        std::string_view vert_filepath,
        std::string_view frag_filepath,
        const bt_pipeline_config_info config_info);
    bt_pipeline(const bt_pipeline&) = delete;
    ~bt_pipeline();

    void operator=(const bt_pipeline&) = delete;

  private:
    void create_graphics_pipeline(std::string_view vert_filepath,
        std::string_view frag_filepath,
        const bt_pipeline_config_info config_info);

    void create_shader_module(const std::vector<char>& code, VkShaderModule* shader_module);

    bt_device& device;
    VkPipeline graphics_pipeline;
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
};
} // namespace bt

#endif // BT_PIPELINE_HPP
