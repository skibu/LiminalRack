/* C-code for handling multitouch using Wayland. Since this is for Wayland 
 * it only works on Linux. */
#ifdef __linux__

// Special Wayland includes
extern "C" {
#include "internal.h"
#include "wayland-client-protocol.h"

// For text input support
#include <wayland-client.h>
#include "window/wayland-text-input-unstable-v3-client-protocol.h"
}

#include <logger.hpp>
#include <window/WaylandTouch.hpp>

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
    struct wl_registry* wl_registry;
    struct wl_seat* wl_seat;
    struct wl_display* wl_display;
    struct wl_touch* wl_touch;
    struct touch_event touch_event;
};


/** Callback that is called as soon as a touch down event occurs */
static void wl_touch_down(void* data, struct wl_touch* wl_touch,
                          uint32_t serial, uint32_t time,
                          struct wl_surface* surface, int32_t id, wl_fixed_t x,
                          wl_fixed_t y) {
    TRACE("Wayland touch down event: id=%d, x=%f, y=%f", id, wl_fixed_to_double(x),
          wl_fixed_to_double(y));

    rack::window::WaylandTouch::downEventCallback(serial, time, id, wl_fixed_to_double(x),
                                    wl_fixed_to_double(y));
}

/** Callback that is called as soon as a touch up event occurs */
static void wl_touch_up(void* data, struct wl_touch* wl_touch, uint32_t serial,
                        uint32_t time, int32_t id) {
    TRACE("Wayland touch up event: id=%d", id);

    rack::window::WaylandTouch::upEventCallback(serial, time, id);
}

/** Callback that is called as soon as a touch motion event occurs. Get
 * a separate event for each finger touching and moving on the screen.
 */
static void wl_touch_motion(void* data, struct wl_touch* wl_touch,
                            uint32_t time, int32_t id, wl_fixed_t x,
                            wl_fixed_t y) {
    TRACE("Wayland touch motion event: id=%d, x=%f, y=%f", id, wl_fixed_to_double(x),
          wl_fixed_to_double(y));

    rack::window::WaylandTouch::motionEventCallback(
        time, id, wl_fixed_to_double(x), wl_fixed_to_double(y));
}

/** Callback that is called as soon as a touch cancel event occurs.
 * Not sure when this actually happens.
*/
static void wl_touch_cancel(void* data, struct wl_touch* wl_touch) {
    TRACE("Wayland touch cancel event");
}

/** Called when get an event that specifies the shape of the touch contact.
 * Unfortunately don't seem to ever receive this kind of event. If did could
 * use it to implement pressure sensitivity.
 */
static void wl_touch_shape(void* data, struct wl_touch* wl_touch, int32_t id,
                           wl_fixed_t major, wl_fixed_t minor) {
    TRACE("Wayland touch shape event: id=%d, major=%f, minor=%f", id,
          wl_fixed_to_double(major), wl_fixed_to_double(minor));

    rack::window::WaylandTouch::shapeEventCallback(
        id, wl_fixed_to_double(major), wl_fixed_to_double(minor));
}

/** Called when get an event that specifies the shape & orientation of the touch contact.
 * Unfortunately don't seem to ever receive this kind of event. If did could
 * possibly use it to implement something fancy, but doesn't seem that important.
 */
static void wl_touch_orientation(void* data, struct wl_touch* wl_touch,
                                 int32_t id, wl_fixed_t orientation) {
    TRACE("Wayland touch orientation event: id=%d, orientation=%f", id,
          wl_fixed_to_double(orientation));

    rack::window::WaylandTouch::orientationEventCallback(id, wl_fixed_to_double(orientation));
}

/** Callback that is called as soon as a touch frame event occurs, which
 * is supposed to be when there are multiple touch events aggregated together
 * and that can be processed at once. 
 */
static void wl_touch_frame(void* data, struct wl_touch* wl_touch) {
    struct client_state* client_state = (struct client_state*)data;
    struct touch_event* touchEvent = &client_state->touch_event;
    TRACE("Wayland touch frame event: @ %d", touchEvent->time);

    rack::window::WaylandTouch::frameEventCallback();
}

static const struct wl_touch_listener wl_touch_listener = {
    .down = wl_touch_down,
    .up = wl_touch_up,
    .motion = wl_touch_motion,
    .frame = wl_touch_frame,
    .cancel = wl_touch_cancel,
    .shape = wl_touch_shape,
    .orientation = wl_touch_orientation,
};

/////////// TEXT INPUT STUFF ///////////

/** Listen for enter event to know which surface is focused
 */
void text_input_enter(void *data, struct zwp_text_input_v3 *text_input,
                      struct wl_surface *surface) {
    TRACE("xxxxxxxxxxxxxxxxxxx Wayland text input enter event");

    // Enable text input for this surface
    zwp_text_input_v3_enable(text_input);
    zwp_text_input_v3_commit(text_input);

    // // FIXME
    // // Set content type/purpose (e.g., normal text)
    // zwp_text_input_v3_set_content_type(text_input,
    //                                    ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
    //                                    ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL);
    
    // // Update state to apply changes
    // zwp_text_input_v3_update_state(text_input,
    //                                ZWP_TEXT_INPUT_V3_UPDATE_STATE_CHANGE_CAUSE_INPUT_METHOD);
    
    // // Commit changes to compositor
    // wl_display_flush(reinterpret_cast<wl_display*>(data));
}

/** Listen for leave event to know when surface is unfocused 
 */
void text_input_leave(void *data, struct zwp_text_input_v3 *text_input,
                      struct wl_surface *surface) {
    TRACE("zzzzzzzzzzzzzzzzzzzz Wayland text input leave event");

    // Disable text input for this surface
    zwp_text_input_v3_disable(text_input);  
}

/*
	void (*commit_string)(void *data,
			      struct zwp_text_input_v3 *zwp_text_input_v3,
			      const char *text);
*/
void text_input_commit_string(void *data, struct zwp_text_input_v3 *text_input,
                      const char *text) {
    TRACE("zzzzzzzzzzzzzzzzzzzz Wayland text input commit string event char='%s'", text);
}

/*
	void (*done)(void *data,
		     struct zwp_text_input_v3 *zwp_text_input_v3,
		     uint32_t serial);
*/
void text_input_done(void *data, struct zwp_text_input_v3 *text_input,
                      uint32_t serial) {
    TRACE("zzzzzzzzzzzzzzzzzzzz Wayland text input done event");
}

static const struct zwp_text_input_v3_listener text_input_listener = {
    .enter = text_input_enter,
    .leave = text_input_leave,
    .preedit_string = nullptr,
    .commit_string = text_input_commit_string,
    .delete_surrounding_text = nullptr,
    .done = text_input_done
};

/////////// SEAT STUFF ///////////

/** Called when seat capabilities change. Used to determine if should
 * acquire touch interface.
 */
static void wl_seat_capabilities(void* data, struct wl_seat* wl_seat,
                                 uint32_t capabilities) {
    // capabilities indicates if seat has keyboard, pointer, or touch
    TRACE("Setting Wayland seat capabilities: %d", capabilities);

    struct client_state* state = (struct client_state*)data;

    bool have_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;
    if (have_touch && state->wl_touch == NULL) {
        state->wl_touch = wl_seat_get_touch(state->wl_seat);
        DEBUG("Acquiring wl_touch interface via wl_touch_add_listener()");
        wl_touch_add_listener(state->wl_touch, &wl_touch_listener, state);
    } else if (!have_touch && state->wl_touch != NULL) {
        DEBUG("Releasing wl_touch interface via wl_touch_release()");
        wl_touch_release(state->wl_touch);
        state->wl_touch = NULL;
    }
}

/** Called when seat name is set. Dont need to do anything here */
static void wl_seat_name(void* data, struct wl_seat* wl_seat,
                         const char* name) {
    TRACE("Seat name: %s\n", name);
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name = wl_seat_name,
};

//////////// REGISTRY STUFF ///////////

// Registry code is copied from https://wayland-book.com/xdg-shell-basics/example-code.html
static void registry_global(void* data, struct wl_registry* wl_registry,
                            uint32_t name, const char* interface,
                            uint32_t version) {
    TRACE("===== Adding Wayland registry global: interface %s (version %d) name %d",
          interface, version, name);

    struct client_state* state = (struct client_state*)data;

    // Look for wl_seat interface to get touch events
    if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->wl_seat = (struct wl_seat*)wl_registry_bind(
            state->wl_registry, name, &wl_seat_interface, version);
        TRACE("Adding Wayland registry global: interface %s (version %d)", interface,
              version);
        wl_seat_add_listener(state->wl_seat, &wl_seat_listener, data);

        // FIXME Try to get text input manager here too, now that have seat
        TRACE(
            "Binding to zwp_text_input_manager_v3 interface");

        // Set up text input manager
        struct zwp_text_input_manager_v3* text_input_manager =
            (struct zwp_text_input_manager_v3*)wl_registry_bind(
                state->wl_registry, 14/*name*/, &zwp_text_input_manager_v3_interface, 1/*version*/);

        // Create text input object and add listener
        struct zwp_text_input_v3* text_input =
            zwp_text_input_manager_v3_get_text_input(text_input_manager,
                                                     state->wl_seat);
        zwp_text_input_v3_add_listener(text_input, &text_input_listener,
                                       state->wl_display);


        return;
    }

    // Look for zwp_text_input_manager_v3 interface to get text input support
    // so that can use virtual keyboard on touch devices
    if (strcmp(interface, zwp_text_input_manager_v3_interface.name) == 999) {
        TRACE(
            "Binding to zwp_text_input_manager_v3 interface");

        // Set up text input manager
        struct zwp_text_input_manager_v3* text_input_manager =
            (struct zwp_text_input_manager_v3*)wl_registry_bind(
                state->wl_registry, name, &zwp_text_input_manager_v3_interface, version);

        // Create text input object and add listener
        struct zwp_text_input_v3* text_input =
            zwp_text_input_manager_v3_get_text_input(text_input_manager,
                                                     state->wl_seat);
        zwp_text_input_v3_add_listener(text_input, &text_input_listener,
                                       state->wl_display);

        return;
    }
}

/** For when removing something from registry. Don't need to do anything here */
static void registry_global_remove(void* data, struct wl_registry* wl_registry,
                                   uint32_t name) {
    TRACE("Removing Wayland registry global: %d\n", name);
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};


///////// INITIALIZATION STUFF ///////////

/** Initializes the C-code for Wayland multitouch support. Needs to be called
 * externally. */
extern "C" {
void waylandMultitouchInit() {
    INFO("Initializing Wayland touch support...");

    // Get access to Wayland registry
    static struct client_state state = {};
    state.wl_display = _glfw.wl.display; 
    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

/*    
    // FIXME
    // Set up text input manager
    struct zwp_text_input_manager_v3* text_input_manager =
        (struct zwp_text_input_manager_v3*)wl_registry_bind(
            state.wl_registry, 1, &zwp_text_input_manager_v3_interface, 1);

    // Create text input object and add listener
    struct zwp_text_input_v3* text_input =
        zwp_text_input_manager_v3_get_text_input(text_input_manager,
                                                 state.wl_seat);
    zwp_text_input_v3_add_listener(text_input, &text_input_listener,
                                   state.wl_display);
*/
    DEBUG("Done initializing Wayland touch support.");
}

}  // extern "C"

#endif // __linux__