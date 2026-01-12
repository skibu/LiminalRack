/* C-code for handling multitouch using Wayland. Since this is for Wayland 
 * it only works on Linux. */
#ifdef __linux__

extern "C" {
#include "internal.h"
#include "wayland-client-protocol.h"
}

#include <stdbool.h>
#include <stdio.h>  // for stderr
#include <string.h>
#include <logger.hpp>

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
    // struct wl_shm *wl_shm;
    // struct wl_compositor *wl_compositor;
    // struct xdg_wm_base *xdg_wm_base;
    struct wl_registry* wl_registry;
    struct wl_seat* wl_seat;
    struct wl_display* wl_display;
    struct wl_touch* wl_touch;
    struct touch_event touch_event;
};


static struct touch_point* get_touch_point(struct client_state* client_state,
                                           int32_t id) {
    DEBUG("FIXME Getting touch point for id %d", id);
    fprintf(stderr, "FIXME Getting touch point for id %d\n", id);

    struct touch_event* touch = &client_state->touch_event;
    const size_t nmemb = sizeof(touch->points) / sizeof(struct touch_point);
    int invalid = -1;
    for (size_t i = 0; i < nmemb; ++i) {
        if (touch->points[i].id == id) {
            DEBUG("Found touch point for id %d", id);
            return &touch->points[i];
        }
        if (invalid == -1 && !touch->points[i].valid) {
            invalid = i;
        }
    }
    if (invalid == -1) {
        return NULL;
    }
    touch->points[invalid].valid = true;
    touch->points[invalid].id = id;
    return &touch->points[invalid];
}

/** Callback that is called as soon as a touch down event occurs */
static void wl_touch_down(void* data, struct wl_touch* wl_touch,
                          uint32_t serial, uint32_t time,
                          struct wl_surface* surface, int32_t id, wl_fixed_t x,
                          wl_fixed_t y) {
    DEBUG("FIXME Touch down event: id=%d, x=%f, y=%f", id, wl_fixed_to_double(x),
          wl_fixed_to_double(y));
    fprintf(stderr, "FIXME Touch down event: id=%d, x=%f, y=%f\n", id,
            wl_fixed_to_double(x), wl_fixed_to_double(y));
    struct client_state* client_state = (struct client_state*)data;
    struct touch_point* point = get_touch_point(client_state, id);
    if (point == NULL) {
        return;
    }
    point->event_mask |= TOUCH_EVENT_UP;
    point->surface_x = wl_fixed_to_double(x),
    point->surface_y = wl_fixed_to_double(y);
    client_state->touch_event.time = time;
    client_state->touch_event.serial = serial;
}

/** Callback that is called as soon as a touch up event occurs */
static void wl_touch_up(void* data, struct wl_touch* wl_touch, uint32_t serial,
                        uint32_t time, int32_t id) {
    DEBUG("FIXME Touch up event: id=%d", id);
    fprintf(stderr, "FIXME Touch up event: id=%d\n", id);

    struct client_state* client_state = (struct client_state*)data;
    struct touch_point* point = get_touch_point(client_state, id);
    if (point == NULL) {
        return;
    }
    point->event_mask |= TOUCH_EVENT_UP;
}

/** Callback that is called as soon as a touch motion event occurs. Get
 * a separate event for each finger touching and moving on the screen.
 */
static void wl_touch_motion(void* data, struct wl_touch* wl_touch,
                            uint32_t time, int32_t id, wl_fixed_t x,
                            wl_fixed_t y) {
    DEBUG("FIXME Touch motion event: id=%d, x=%f, y=%f", id, wl_fixed_to_double(x),
          wl_fixed_to_double(y));
    fprintf(stderr, "FIXME Touch motion event: id=%d, x=%f, y=%f\n", id,
            wl_fixed_to_double(x), wl_fixed_to_double(y));

    struct client_state* client_state = (struct client_state*)data;
    struct touch_point* point = get_touch_point(client_state, id);
    if (point == NULL) {
        return;
    }
    point->event_mask |= TOUCH_EVENT_MOTION;
    point->surface_x = x, point->surface_y = y;
    client_state->touch_event.time = time;
}

/** Callback that is called as soon as a touch cancel event occurs.
 * Not sure when this actually happens.
*/
static void wl_touch_cancel(void* data, struct wl_touch* wl_touch) {
    DEBUG("FIXME Touch cancel event");
    fprintf(stderr, "FIXME Touch cancel event\n");

    struct client_state* client_state = (struct client_state*)data;
    client_state->touch_event.event_mask |= TOUCH_EVENT_CANCEL;
}

/** Called when get an event that specifies the shape of the touch contact.
 * Unfortunately don't seem to ever receive this kind of event. If did could
 * use it to implement pressure sensitivity.
 */
static void wl_touch_shape(void* data, struct wl_touch* wl_touch, int32_t id,
                           wl_fixed_t major, wl_fixed_t minor) {
    DEBUG("FIXME Touch shape event: id=%d, major=%f, minor=%f", id,
          wl_fixed_to_double(major), wl_fixed_to_double(minor));
    fprintf(stderr, "FIXME Touch shape event: id=%d, major=%f, minor=%f\n", id,
            wl_fixed_to_double(major), wl_fixed_to_double(minor));

    struct client_state* client_state = (struct client_state*)data;
    struct touch_point* point = get_touch_point(client_state, id);
    if (point == NULL) {
        return;
    }
    point->event_mask |= TOUCH_EVENT_SHAPE;
    point->major = major, point->minor = minor;
}

/** Called when get an event that specifies the shape & orientation of the touch contact.
 * Unfortunately don't seem to ever receive this kind of event. If did could
 * possibly use it to implement something fancy, but doesn't seem that important.
 */
static void wl_touch_orientation(void* data, struct wl_touch* wl_touch,
                                 int32_t id, wl_fixed_t orientation) {
    DEBUG("Touch orientation event: id=%d, orientation=%f", id,
          wl_fixed_to_double(orientation));
    fprintf(stderr, "FIXME Touch orientation event: id=%d, orientation=%f\n", id,
            wl_fixed_to_double(orientation));

    struct client_state* client_state = (struct client_state*)data;
    struct touch_point* point = get_touch_point(client_state, id);
    if (point == NULL) {
        return;
    }
    point->event_mask |= TOUCH_EVENT_ORIENTATION;
    point->orientation = orientation;
}

/** Callback that is called as soon as a touch frame event occurs, which
 * is supposed to be when there are multiple touch events aggregated together
 * and that can be processed at once. 
 * 
 * FIXME Don't seem to be getting valid points though.
 */
static void wl_touch_frame(void* data, struct wl_touch* wl_touch) {
    struct client_state* client_state = (struct client_state*)data;
    struct touch_event* touchEvent = &client_state->touch_event;
    const size_t nmemb =
        sizeof(touchEvent->points) / sizeof(struct touch_point);
    fprintf(stderr, "Touch frame event time %d:\n", touchEvent->time);
    DEBUG("Touch frame event: @ %d", touchEvent->time);

    for (size_t i = 0; i < nmemb; ++i) {
        struct touch_point* point = &touchEvent->points[i];
        if (!point->valid) {
            continue;
        }
        fprintf(stderr, "point %d: ", touchEvent->points[i].id);
        DEBUG("Touch frame event: point %d: ", touchEvent->points[i].id);

        if (point->event_mask & TOUCH_EVENT_DOWN) {
            fprintf(stderr, "down %f,%f ", wl_fixed_to_double(point->surface_x),
                    wl_fixed_to_double(point->surface_y));
            DEBUG("Touch frame event: point %d: down %f,%f",
                  touchEvent->points[i].id,
                  wl_fixed_to_double(point->surface_x),
                  wl_fixed_to_double(point->surface_y));
        }

        if (point->event_mask & TOUCH_EVENT_UP) {
            fprintf(stderr, "up ");
            DEBUG("Touch frame event: point %d: up", touchEvent->points[i].id);
        }

        if (point->event_mask & TOUCH_EVENT_MOTION) {
            fprintf(stderr, "motion %f,%f ",
                    wl_fixed_to_double(point->surface_x),
                    wl_fixed_to_double(point->surface_y));
            DEBUG("Touch frame event: point %d: motion %f,%f",
                  touchEvent->points[i].id,
                  wl_fixed_to_double(point->surface_x),
                  wl_fixed_to_double(point->surface_y));
        }

        if (point->event_mask & TOUCH_EVENT_SHAPE) {
            fprintf(stderr, "shape %fx%f ", wl_fixed_to_double(point->major),
                    wl_fixed_to_double(point->minor));
            DEBUG("Touch frame event: point %d: shape %fx%f",
                  touchEvent->points[i].id, wl_fixed_to_double(point->major),
                  wl_fixed_to_double(point->minor));
        }

        if (point->event_mask & TOUCH_EVENT_ORIENTATION) {
            fprintf(stderr, "orientation %f ",
                    wl_fixed_to_double(point->orientation));
            DEBUG("Touch frame event: point %d: orientation %f",
                  touchEvent->points[i].id,
                  wl_fixed_to_double(point->orientation));
        }

        point->valid = false;
        fprintf(stderr, "\n");
    }
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

/////////// SEAT STUFF ///////////

/** Called when seat capabilities change. Used to determine if should
 * acquire touch interface.
 */
static void wl_seat_capabilities(void* data, struct wl_seat* wl_seat,
                                 uint32_t capabilities) {
    // capabilities indicates if seat has keyboard, pointer, or touch
    DEBUG("Setting seat capabilities: %d", capabilities);

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
    struct client_state* state = (struct client_state*)data;
    if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->wl_seat = (struct wl_seat*)wl_registry_bind(
            state->wl_registry, name, &wl_seat_interface, 7 /* seat version */);
        DEBUG("Adding registry global: interface %s (version %d)", interface,
              version);
        wl_seat_add_listener(state->wl_seat, &wl_seat_listener, data);
    }
}

/** For when removing something from registry. Don't need to do anything here */
static void registry_global_remove(void* data, struct wl_registry* wl_registry,
                                   uint32_t name) {
    DEBUG("Removing registry global: %d\n", name);
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

    // FIXME need capabilities, state, and wl_touch_listener
    // Get access to Wayland registry
    static struct client_state state = {};
    state.wl_display = _glfw.wl.display;
    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

    DEBUG("Done initializing Wayland touch support.");
}
}  // extern "C"

#endif // __linux__