#include <context.hpp>
#include <common.hpp>
#include <window/WaylandTouch.hpp>
#include <chrono>

namespace rack {
namespace window {

long WaylandTouchEvent::getCurrentTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void WaylandTouch::init() {
#ifdef __linux__
    INFO("Initializing Wayland touch support...");
    waylandMultitouchInit();
#endif
}

void WaylandTouch::processEvents() {
    DEBUG("Processing Wayland touch events...");
    
    // FIXME implement event processing
    for (int id = 0; id < NUM_TOUCHPOINTS; ++id) {
        auto& queue = eventQueues_[id];
        if (!queue.empty()) continue;

        // Process all events for this touch point id
        DEBUG("Processing events for touch id %d", id);
        while (!queue.empty()) {
            WaylandTouchEvent event = queue.front();
            queue.pop();

            // Process event
            DEBUG("Processing touch event: id=%d, type=%d", event.id_,
                  static_cast<int>(event.eventType_));

            // FIXME for now just handle a button press
            if (event.getEventType() == WaylandTouchEvent::TOUCH_DOWN) {
                DEBUG("Touch down event at (%d, %d)", event.getX(),
                      event.getY());
                getEvent()->handleButton(math::Vec(event.getX(), event.getY()),
                                         GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
            }
        }
    }
}

void WaylandTouch::addEventToQueue(const WaylandTouchEvent& event) {
    if (event.id_ < 0 || event.id_ >= NUM_TOUCHPOINTS) {
        ERROR("WaylandTouch::addEventToQueue: Invalid touch id %d", event.id_);
        return;
    }
    eventQueues_[event.id_].push(event);
}

// Allocate static member
std::queue<WaylandTouchEvent> WaylandTouch::eventQueues_[NUM_TOUCHPOINTS];

}  // namespace window
}  // namespace rack