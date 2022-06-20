#ifndef BT_PIPELINE_HPP
#define BT_PIPELINE_HPP

#include <string_view>
#include <vector>

namespace bt {
class bt_pipeline {
  public:
    bt_pipeline(std::string_view vert_filepath, std::string_view frag_filepath);

  private:
    void create_graphics_pipeline(std::string_view vert_filepath, std::string_view frag_filepath);
};
} // namespace bt

#endif // BT_PIPELINE_HPP
