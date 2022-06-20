#ifndef APP_HPP
#define APP_HPP

#include "bt_window.hpp"

namespace bt {
class app {
  public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 720;

    void run();

  private:
    bt_window window { WIDTH, HEIGHT, "Breakable Toy" };
};
} // namespace bt

#endif // APP_HPP
