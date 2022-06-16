#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <array>
#include <format>
#include <iostream>

int main(int argc, char* argv[])
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::array<spdlog::sink_ptr, 1> sinks { console_sink };

    auto logger = std::make_shared<spdlog::logger>("BT", sinks.begin(), sinks.end());
    logger->set_pattern("[%^%l%$] %v");
    logger->set_level(spdlog::level::info);

    spdlog::set_default_logger(logger);

    SPDLOG_INFO("Welcome to {}!", "Breakable Toy");
    SPDLOG_DEBUG("Shoudl not show");

    return EXIT_SUCCESS;
}
