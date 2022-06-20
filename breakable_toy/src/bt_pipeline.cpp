#include "bt_pipeline.hpp"

#include "bt_filesystem.hpp"
#include "bt_logger.hpp"

namespace bt {
bt_pipeline::bt_pipeline(std::string_view vert_filepath, std::string_view frag_filepath)
{
    create_graphics_pipeline(vert_filepath, frag_filepath);
}

void bt_pipeline::create_graphics_pipeline(
    std::string_view vert_filepath, std::string_view frag_filepath)
{
    auto vert_code = bt_filesystem::read_file(vert_filepath);
    auto frag_code = bt_filesystem::read_file(frag_filepath);

    SPDLOG_INFO("Vertex shader code size: {}", vert_code.size());
    SPDLOG_INFO("Fragment shader code size: {}", frag_code.size());
}
} // namespace bt
