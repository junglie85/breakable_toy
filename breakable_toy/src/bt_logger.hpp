#ifndef BT_LOGGER_HPP
#define BT_LOGGER_HPP

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace bt {
class bt_logger {
  public:
    bt_logger(spdlog::level::level_enum max_level);
    bt_logger(const bt_logger&) = delete;
    bt_logger(bt_logger&&) = delete;
    ~bt_logger();

    bt_logger& operator=(const bt_logger&) = delete;
    bt_logger& operator=(bt_logger&&) = delete;

  private:
    void init_logger();

    spdlog::level::level_enum max_level;
    const char* logger_name;
};
} // namespace bt

#endif // BT_LOGGER_HPP
