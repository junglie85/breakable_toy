#include "app.hpp"
#include "bt_logger.hpp"

#include <cstdlib>
#include <stdexcept>

int main(int argc, char* argv[])
{
    bt::bt_logger logger { spdlog::level::trace };
    bt::app app {};

    try {
        app.run();
    } catch (const std::exception& e) {
        SPDLOG_CRITICAL(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
