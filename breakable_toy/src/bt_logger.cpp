#include "bt_logger.hpp"

#include <array>

namespace bt {
bt_logger::bt_logger(spdlog::level::level_enum max_level) :
    max_level { max_level }, logger_name("BT")
{
    init_logger();
}

bt_logger::~bt_logger() { spdlog::get(logger_name)->flush(); }

void bt_logger::init_logger()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::array<spdlog::sink_ptr, 1> sinks { console_sink };

    auto logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
    logger->set_pattern("[%^%l%$] %v");
    logger->set_level(max_level);

    spdlog::set_default_logger(logger);
}
} // namespace bt
