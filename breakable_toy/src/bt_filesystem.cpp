#include "bt_filesystem.hpp"

#include "bt_logger.hpp"

#include <fmt/core.h>

#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace bt {
void bt_filesystem::init(const char* exe_path)
{
    auto& instance = get_instance();

    auto exe_dir = fs::path(exe_path);
    if (fs::is_regular_file(exe_dir)) {
        exe_dir.remove_filename();
    }

    instance.base_path = fs::canonical(exe_dir);

    SPDLOG_TRACE("filesystem base path is {}", exe_dir.c_str());
}

std::vector<char> bt_filesystem::read_file(std::string_view filepath)
{
    std::ifstream file(absolute_path_to(filepath), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("failed to open file at {}", filepath));
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

fs::path bt_filesystem::absolute_path_to(std::string_view filepath)
{
    return fs::canonical(get_base_path().append(filepath));
}

bt_filesystem& bt_filesystem::get_instance()
{
    static bt_filesystem instance;
    return instance;
}
}; // namespace bt
