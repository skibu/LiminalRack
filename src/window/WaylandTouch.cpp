#include <window/WaylandTouch.hpp>
#include <common.hpp>

namespace rack {
namespace window {

#ifdef __linux__
void WaylandTouch::init() {
    INFO("Flushing Initializing Wayland touch support...");

    waylandMultitouchInit();
}

#else

/** Not using Wayland so don't do anything here */
void WaylandTouch::init() {}

#endif

}  // namespace window
}  // namespace rack