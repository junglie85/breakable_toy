#include "bt_pipeline.hpp"

#include "bt_filesystem.hpp"
#include "bt_logger.hpp"

#include <stdexcept>

namespace bt {
bt_pipeline_config_info bt_pipeline::default_pipeline_config_info(uint32_t width, uint32_t height)
{
    bt_pipeline_config_info config_info {};

    return config_info;
}

bt_pipeline::bt_pipeline(bt_device& device,
    std::string_view vert_filepath,
    std::string_view frag_filepath,
    const bt_pipeline_config_info config_info) :
    device { device }
{
    create_graphics_pipeline(vert_filepath, frag_filepath, config_info);
}

void bt_pipeline::create_graphics_pipeline(std::string_view vert_filepath,
    std::string_view frag_filepath,
    const bt_pipeline_config_info config_info)
{
    auto vert_code = bt_filesystem::read_file(vert_filepath);
    auto frag_code = bt_filesystem::read_file(frag_filepath);

    SPDLOG_INFO("Vertex shader code size: {}", vert_code.size());
    SPDLOG_INFO("Fragment shader code size: {}", frag_code.size());
}

void bt_pipeline::create_shader_module(const std::vector<char>& code, VkShaderModule shader_module)
{
    VkShaderModuleCreateInfo create_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    if (vkCreateShaderModule(device.device(), &create_info, device.allocator(), &shader_module)
        != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
}
} // namespace bt
