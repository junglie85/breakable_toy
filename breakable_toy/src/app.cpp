#include "app.hpp"

namespace bt {
void app::run()
{
    while (!window.should_close()) {
        glfwPollEvents();
    }
}
} // namespace bt
