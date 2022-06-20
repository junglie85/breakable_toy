#ifndef BT_FILESYSTEM_HPP
#define BT_FILESYSTEM_HPP

#include <filesystem>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace bt {
class bt_filesystem {
  public:
    static void init(const char* exe_dir);
    static std::vector<char> read_file(std::string_view filepath);
    static fs::path absolute_path_to(std::string_view filepath);

  private:
    bt_filesystem() = default;

    static bt_filesystem& get_instance();
    static inline fs::path get_base_path() { return get_instance().base_path; }

    fs::path base_path;
};
} // namespace bt

#endif // BT_FILESYSTEM_HPP
