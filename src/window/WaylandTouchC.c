/* C-code for handling multitouch using Wayland. Since this is for Wayland 
 * it only works on Linux. */
#ifdef __linux__

#include "internal.h"
#include "wayland-client-protocol.h"
                      
#include <stdbool.h>
#include <stdio.h>  // for stderr
#include <string.h>



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

struct client_state
{
        // struct wl_shm *wl_shm;
        // struct wl_compositor *wl_compositor;
        // struct xdg_wm_base *xdg_wm_base;
        struct wl_registry *wl_registry;
        struct wl_seat *wl_seat;
        struct wl_display *wl_display;
        struct wl_touch *wl_touch;
        struct touch_event touch_event;
};

static struct touch_point *
get_touch_point(struct client_state *client_state, int32_t id)
{
        //DEBUG("Getting touch point for id %d", id);

       struct touch_event *touch = &client_state->touch_event;
       const size_t nmemb = sizeof(touch->points) / sizeof(struct touch_point);
       int invalid = -1;
       for (size_t i = 0; i < nmemb; ++i) {
               if (touch->points[i].id == id) {
                       //DEBUG("Found touch point for id %d", id);
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

static void
wl_touch_down(void *data, struct wl_touch *wl_touch, uint32_t serial,
               uint32_t time, struct wl_surface *surface, int32_t id,
               wl_fixed_t x, wl_fixed_t y)
{
       //DEBUG("Touch down event: id=%d, x=%f, y=%f", id, wl_fixed_to_double(x), wl_fixed_to_double(y));
        fprintf(stderr, "FIXME Touch down event: id=%d, x=%f, y=%f\n", id, wl_fixed_to_double(x), wl_fixed_to_double(y));
        fflush(stderr);
       struct client_state *client_state = data;
       struct touch_point *point = get_touch_point(client_state, id);
       if (point == NULL) {
               return;
       }
       point->event_mask |= TOUCH_EVENT_UP;
       point->surface_x = wl_fixed_to_double(x),
               point->surface_y = wl_fixed_to_double(y);
       client_state->touch_event.time = time;
       client_state->touch_event.serial = serial;
}

static void
wl_touch_up(void *data, struct wl_touch *wl_touch, uint32_t serial,
               uint32_t time, int32_t id)
{
       //DEBUG("Touch up event: id=%d", id);
        fprintf(stderr, "Touch up event: id=%d\n", id);
        fflush(stderr);

       struct client_state *client_state = data;
       struct touch_point *point = get_touch_point(client_state, id);
       if (point == NULL) {
               return;
       }
       point->event_mask |= TOUCH_EVENT_UP;
}

static void
wl_touch_motion(void *data, struct wl_touch *wl_touch, uint32_t time,
               int32_t id, wl_fixed_t x, wl_fixed_t y)
{
        //DEBUG("Touch motion event: id=%d, x=%f, y=%f", id, wl_fixed_to_double(x), wl_fixed_to_double(y));
        fprintf(stderr, "FIXME Touch motion event: id=%d, x=%f, y=%f\n", id, wl_fixed_to_double(x), wl_fixed_to_double(y));
        fflush(stderr);

       struct client_state *client_state = data;
       struct touch_point *point = get_touch_point(client_state, id);
       if (point == NULL) {
               return;
       }
       point->event_mask |= TOUCH_EVENT_MOTION;
       point->surface_x = x, point->surface_y = y;
       client_state->touch_event.time = time;
}

static void
wl_touch_cancel(void *data, struct wl_touch *wl_touch)
{
       //DEBUG("Touch cancel event");
        fprintf(stderr, "Touch cancel event\n");
        fflush(stderr);

       struct client_state *client_state = data;
       client_state->touch_event.event_mask |= TOUCH_EVENT_CANCEL;
}

static void
wl_touch_shape(void *data, struct wl_touch *wl_touch,
               int32_t id, wl_fixed_t major, wl_fixed_t minor)
{
       //DEBUG("Touch shape event: id=%d, major=%f, minor=%f", id, wl_fixed_to_double(major), wl_fixed_to_double(minor));
        fprintf(stderr, "Touch shape event: id=%d, major=%f, minor=%f\n", id, wl_fixed_to_double(major), wl_fixed_to_double(minor));
        fflush(stderr);

       struct client_state *client_state = data;
       struct touch_point *point = get_touch_point(client_state, id);
       if (point == NULL) {
               return;
       }
       point->event_mask |= TOUCH_EVENT_SHAPE;
       point->major = major, point->minor = minor;
}

static void
wl_touch_orientation(void *data, struct wl_touch *wl_touch,
               int32_t id, wl_fixed_t orientation)
{
       //DEBUG("Touch orientation event: id=%d, orientation=%f", id, wl_fixed_to_double(orientation));
        fprintf(stderr, "Touch orientation event: id=%d, orientation=%f\n", id, wl_fixed_to_double(orientation));
        fflush(stderr);

       struct client_state *client_state = data;
       struct touch_point *point = get_touch_point(client_state, id);
       if (point == NULL) {
               return;
       }
       point->event_mask |= TOUCH_EVENT_ORIENTATION;
       point->orientation = orientation;
}

static void
wl_touch_frame(void *data, struct wl_touch *wl_touch)
{
       //DEBUG("Touch frame event");

       struct client_state *client_state = data;
       struct touch_event *touchEvent = &client_state->touch_event;
       const size_t nmemb = sizeof(touchEvent->points) / sizeof(struct touch_point);
       fprintf(stderr, "touch frame event time %d:\n", touchEvent->time);
       //DEBUG("Touch frame event: @ %d", touch->time);

       struct wl_touch *foo = wl_touch;

       for (size_t i = 0; i < nmemb; ++i) {
               struct touch_point *point = &touchEvent->points[i];
               if (!point->valid) {
                    continue;
               }
               fprintf(stderr, "point %d: ", touchEvent->points[i].id);
               fflush(stderr);
               //DEBUG("Touch frame event: point %d: ", touch->points[i].id);

               if (point->event_mask & TOUCH_EVENT_DOWN) {
                       fprintf(stderr, "down %f,%f ",
                                       wl_fixed_to_double(point->surface_x),
                                       wl_fixed_to_double(point->surface_y));
                          //DEBUG("Touch frame event: point %d: down %f,%f", touch->points[i].id,
                                       //wl_fixed_to_double(point->surface_x),
                                       //wl_fixed_to_double(point->surface_y));
               }

               if (point->event_mask & TOUCH_EVENT_UP) {
                       fprintf(stderr, "up ");
                       //DEBUG("Touch frame event: point %d: up", touch->points[i].id);
               }

               if (point->event_mask & TOUCH_EVENT_MOTION) {
                       fprintf(stderr, "motion %f,%f ",
                                       wl_fixed_to_double(point->surface_x),
                                       wl_fixed_to_double(point->surface_y));
                          //DEBUG("Touch frame event: point %d: motion %f,%f", touch->points[i].id,
                                       //wl_fixed_to_double(point->surface_x),
                                       //wl_fixed_to_double(point->surface_y));
               }

               if (point->event_mask & TOUCH_EVENT_SHAPE) {
                       fprintf(stderr, "shape %fx%f ",
                                       wl_fixed_to_double(point->major),
                                       wl_fixed_to_double(point->minor));
                       //DEBUG("Touch frame event: point %d: shape %fx%f", touch->points[i].id,
                                       //wl_fixed_to_double(point->major),
                                       //wl_fixed_to_double(point->minor));
               }

               if (point->event_mask & TOUCH_EVENT_ORIENTATION) {
                       fprintf(stderr, "orientation %f ",
                                       wl_fixed_to_double(point->orientation));
                       //DEBUG("Touch frame event: point %d: orientation %f", touch->points[i].id,
                                       //wl_fixed_to_double(point->orientation));
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

/////////// SEAT STUFF

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
        //DEBUG("Seat capabilities: %d\n", capabilities);
        fprintf(stderr, "Seat capabilities: %d\n", capabilities);
        fflush(stderr);

        struct client_state *state = data;

        bool have_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;
        if (have_touch && state->wl_touch == NULL)
        {
                state->wl_touch = wl_seat_get_touch(state->wl_seat);
                //DEBUG("Acquiring wl_touch interface via wl_touch_add_listener()");
                wl_touch_add_listener(state->wl_touch,
                                      &wl_touch_listener, state);
        }
        else if (!have_touch && state->wl_touch != NULL)
        {
                //DEBUG("Releasing wl_touch interface via wl_touch_release()");
                wl_touch_release(state->wl_touch);
                state->wl_touch = NULL;
        }
}

static void
wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
       fprintf(stderr, "FIXME seat name: %s\n", name);
       //DEBUG("Seat name: %s\n", name);
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
        //DEBUG("Adding registry global: interface %s (version %d)\n", interface, version);
        fprintf(stderr, "Adding registry global: interface %s (version %d)\n", interface, version);
        fflush(stderr);

        struct client_state *state = data;
        if (strcmp(interface, wl_seat_interface.name) == 0)
        {
                // uint32_t name = 1; // FIXME get actual name from registry
                state->wl_seat = wl_registry_bind(
                    state->wl_registry, name, &wl_seat_interface, 7 /* seat version */);
                //DEBUG("Adding wl_seat listener via wl_seat_add_listener()");
                wl_seat_add_listener(state->wl_seat, &wl_seat_listener, data);
        }

//     if (strcmp(interface, wl_shm_interface.name) == 0) {
//         state->wl_shm = wl_registry_bind(
//                 wl_registry, name, &wl_shm_interface, 1);
//     } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
//         state->wl_compositor = wl_registry_bind(
//                 wl_registry, name, &wl_compositor_interface, 4);
//     } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
//         state->xdg_wm_base = wl_registry_bind(
//                 wl_registry, name, &xdg_wm_base_interface, 1);
//         xdg_wm_base_add_listener(state->xdg_wm_base,
//                 &xdg_wm_base_listener, state);
//     }
}

static void
registry_global_remove(void *data,
        struct wl_registry *wl_registry, uint32_t name)
{
        //DEBUG("Removing registry global: %d\n", name);
        fprintf(stderr, "Removing registry global: %d\n", name);
        fflush(stderr);
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};
 


void waylandMultitouchInit() {


    printf("Doing printf within waylandMultitouchInit()\n");
    fflush(stdout);

    //INFO("Initializing Wayland touch support...");
    fprintf(stderr, "Flushig Initializing Wayland touch support...\n");
    fflush(stderr);

    // FIXME need capabilities, state, and wl_touch_listener
    // Get access to Wayland registry
    static struct client_state state = {  };
    /*
    PFN_wl_display_connect wl_display_connect = (PFN_wl_display_connect)
        _glfwPlatformGetModuleSymbol(module, "wl_display_connect");
    state.wl_display = wl_display_connect(NULL);
    */
    state.wl_display = _glfw.wl.display;

    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

    // Get wl_seat from registry
    // uint32_t name = 1; // FIXME get actual name from registry
    // state.wl_seat = wl_registry_bind(
    //                         state.wl_registry, name, &wl_seat_interface, 7 /* seat version */);
    // wl_seat_add_listener(state.wl_seat, &wl_seat_listener, nullptr);




    //DEBUG("Done initializing Wayland touch support."); 
}    

#endif