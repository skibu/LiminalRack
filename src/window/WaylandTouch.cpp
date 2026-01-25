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
    //TRACE("Processing Wayland touch events...");
    
    // FIXME implement event processing
    for (int id = 0; id < NUM_TOUCHPOINTS; ++id) {
        auto& queue = eventQueues_[id];

        // If no events for this touch point id then continue to next touch point
        if (queue.empty()) continue;

        // Process all events for this touch point id
        while (!queue.empty()) {
            WaylandTouchEvent event = queue.front();
            queue.pop();

            // Process event
            DEBUG("Processing touch event: id=%d, type=%d serial=%d time=%ld", event.id_,
                  static_cast<int>(event.eventType_), event.getSerialNumber(), event.getTimeStamp());

            // Handle the touch event by type
            // FIXME - currently only handles single point.
            if (event.getEventType() == WaylandTouchEvent::TOUCH_DOWN) {
                DEBUG("Wayland touch down event at (%d, %d)", event.getX(),
                      event.getY());
                getEvent()->handleButton(math::Vec(event.getX(), event.getY()),
                                         GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
            } else if (event.getEventType() == WaylandTouchEvent::TOUCH_UP) {
                DEBUG("Wayland touch up event at id %d", event.getId());
                getEvent()->handleButton(math::Vec(event.getX(), event.getY()),
                                         GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
            } else if (event.getEventType() == WaylandTouchEvent::TOUCH_MOTION) {
                DEBUG("Wayland touch motion event at (%d, %d)", event.getX(),
                      event.getY());
                getEvent()->handleHover(
                    math::Vec(event.getX(), event.getY()), math::Vec(0, 0));
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

math::Vec WaylandTouch::getLastPosition(int id) {
    if (id < 0 || id >= NUM_TOUCHPOINTS) {
        ERROR("WaylandTouch::getLastPosition: Invalid touch id %d", id);
        return math::Vec();
    }

    return lastTouchPos_[id];
}

void WaylandTouch::storePosition(int id, const math::Vec& pos) {
    if (id < 0 || id >= NUM_TOUCHPOINTS) {
        ERROR("WaylandTouch::storePosition: Invalid touch id %d", id);
        return;
    }

    lastTouchPos_[id] = pos;
}

// Allocate static members
std::queue<WaylandTouchEvent> WaylandTouch::eventQueues_[NUM_TOUCHPOINTS];
math::Vec WaylandTouch::lastTouchPos_[NUM_TOUCHPOINTS];

}  // namespace window
}  // namespace rack