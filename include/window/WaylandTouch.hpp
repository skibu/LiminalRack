#pragma once

extern "C" { void waylandMultitouchInit(); }

namespace rack {
namespace window {

/** For Raspberry Pis with a touch screen can use Wayland to handle touch input.
 * This way can use multi-touch gestures such as pinch to zoom. For other systems
 * need to use regular mouse input handling, which is more limited.
 * 
 * The program should call WaylandTouch::init() during window initialization. The
 * init() function will set up Wayland touch input handling if that ability
 * was included during compilation.  If Wayland touch input handling is not available
 * then the init() function does nothing.
 */
class WaylandTouch {
   public:
    WaylandTouch() {};
    ~WaylandTouch() {};

    static void init();
};    

}  // namespace window
}  // namespace rack