#include <window/WaylandTouch.hpp>
#include <common.hpp>

#include <GLFW/glfw3.h>

// Only include Wayland touch support if enabled during compilation
#ifdef WAYLAND_TOUCHSCREEN_SUPPORT
#include <wayland-client.h> 
#include <wayland-client-protocol.h>
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

struct client_state {
  struct wl_seat* wl_seat;
  struct wl_registry* wl_registry;
  struct wl_seat* wl_seat;
  struct wl_touch* wl_touch;
};

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
       struct client_state *state = data;
       /* TODO */
}

static void
wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
       fprintf(stderr, "seat name: %s\n", name);
}

static const struct wl_seat_listener wl_seat_listener = {
       .capabilities = wl_seat_capabilities,
       .name = wl_seat_name,
};

// Registry code is copied from https://wayland-book.com/xdg-shell-basics/example-code.html
static void
registry_global(void *data, struct wl_registry *wl_registry,
        uint32_t name, const char *interface, uint32_t version)
{
    struct client_state *state = data;
    if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->wl_shm = wl_registry_bind(
                wl_registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->wl_compositor = wl_registry_bind(
                wl_registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(
                wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base,
                &xdg_wm_base_listener, state);
    }
}

static void
registry_global_remove(void *data,
        struct wl_registry *wl_registry, uint32_t name)
{
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

#endif  


void WaylandTouch::init() {

    // Only do something if Wayland touch support is enabled
    #ifdef WAYLAND_TOUCHSCREEN_SUPPORT
    INFO("Initializing Wayland touch support...");

    // FIXME need capabilities, state, and wl_touch_listener
    // Get access to Wayland registry
    struct client_state state = { 0 };
    state.wl_display = wl_display_connect(NULL);
    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

    // Get wl_seat from registry
    // uint32_t name = 1; // FIXME get actual name from registry
    // state.wl_seat = wl_registry_bind(
    //                         state.wl_registry, name, &wl_seat_interface, 7 /* seat version */);
    // wl_seat_add_listener(state.wl_seat, &wl_seat_listener, nullptr);


    bool have_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;
    if (have_touch && state->wl_touch == NULL) {
            state->wl_touch = wl_seat_get_touch(state->wl_seat);
            wl_touch_add_listener(state->wl_touch,
                            &wl_touch_listener, state);
    } else if (!have_touch && state->wl_touch != NULL) {
            wl_touch_release(state->wl_touch);
            state->wl_touch = NULL;
    }

    DEBUG("Done initializing Wayland touch support."); 
    #endif 
}    

}  // namespace window
}  // namespace rack