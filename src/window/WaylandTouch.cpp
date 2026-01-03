#include <window/WaylandTouch.hpp>
#include <common.hpp>

#include <GLFW/glfw3.h>

// Only include Wayland touch support if enabled during compilation
#ifdef WAYLAND_TOUCHSCREEN_SUPPORT
#include <wayland-client.h> 
#endif  

namespace rack {
namespace window {

#ifdef WAYLAND_TOUCHSCREEN_SUPPORT
enum touch_event_mask {
       TOUCH_EVENT_DOWN = 1 << 0,
       TOUCH_EVENT_UP = 1 << 1,
       TOUCH_EVENT_MOTION = 1 << 2,
       TOUCH_EVENT_CANCEL = 1 << 3,
       TOUCH_EVENT_SHAPE = 1 << 4,
       TOUCH_EVENT_ORIENTATION = 1 << 5,
};

struct touch_point {
       bool valid;
       int32_t id;
       uint32_t event_mask;
       wl_fixed_t surface_x, surface_y;
       wl_fixed_t major, minor;
       wl_fixed_t orientation;
};
struct touch_event {
       uint32_t event_mask;
       uint32_t time;
       uint32_t serial;
       struct touch_point points[10];
};
#endif  


void WaylandTouch::init() {

    // Only do something if Wayland touch support is enabled
    #ifdef WAYLAND_TOUCHSCREEN_SUPPORT
    INFO("Initializing Wayland touch support...");
    
    DEBUG("Done initializing Wayland touch support."); 
    #endif 
}    

}  // namespace window
}  // namespace rack