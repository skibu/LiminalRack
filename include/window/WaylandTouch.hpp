#pragma once
#include <logger.hpp>
#include <math.hpp>
#include <queue>

extern "C" {
void waylandMultitouchInit();
}

namespace rack {
namespace window {

/** Touch event received from Wayland. Does not include whether
 * part of a special event such as double click, long hold etc.
 */
class WaylandTouchEvent {
   public:
    enum EventType { TOUCH_DOWN, TOUCH_UP, TOUCH_MOTION, TOUCH_SHAPE, TOUCH_ORIENTATION };

    WaylandTouchEvent(int serialNumber, EventType eventType, int id, int x,
                      int y)
        : serialNumber_(serialNumber),
          eventType_(eventType),
          id_(id),
          x_(x),
          y_(y) {
        // Record time event was created
        time_ = getCurrentTime();
    }

    int getSerialNumber() const { return serialNumber_; }
    long getTimeStamp() const { return time_; }
    EventType getEventType() const { return eventType_; }
    int getId() const { return id_; }
    int getX() const { return x_; }
    int getY() const { return y_; }

    /** Helper function for returning time in milliseconds since epoch */
    static long getCurrentTime();

    // So that WaylandTouch can access private members
    friend class WaylandTouch;

   private:
    // The serial number of the touch event
    int serialNumber_;

    // The time of the touch event in milliseconds
    long time_;

    // The type of touch event
    EventType eventType_;

    // The unique ID of the touch point
    int id_;

    // The x coordinate of the touch point
    int x_;

    // The y coordinate of the touch point
    int y_;
};

class WaylandOrientationEvent : public WaylandTouchEvent {
   public:
    WaylandOrientationEvent(int id, float orientation)
        : WaylandTouchEvent(0, TOUCH_ORIENTATION, id, 0, 0),
          orientation_(orientation) {}

    float getOrientation() const { return orientation_; }

   private:
    float orientation_;
};

class WaylandShapeEvent : public WaylandTouchEvent {
   public:
    WaylandShapeEvent(int id, float major, float minor)
        : WaylandTouchEvent(0, TOUCH_SHAPE, id, 0, 0),
          major_(major),
          minor_(minor) {}

    float getMajor() const { return major_; }
    float getMinor() const { return minor_; }

   private:
    float major_;
    float minor_;
};

/** For Raspberry Pis with a touch screen can use Wayland to handle touch input.
 * This way can use multi-touch gestures such as pinch to zoom. For other
 * systems need to use regular mouse input handling, which is more limited.
 *
 * The program should call WaylandTouch::init() during window initialization.
 * The init() function will set up Wayland touch input handling if that ability
 * was included during compilation.  If Wayland touch input handling is not
 * available then the init() function does nothing.
 */
class WaylandTouch {
   private:
    // Singleton class
    WaylandTouch() {};
    const static WaylandTouch singleton_;

   public:
    /** If Wayland multitouch support is available then initializes it.
     * Otherwise does nothing.
     */
    static void init();

    /** To be called every frame to process any pending touch events.
     * Used for processing raw touch events into things like double clicks, etc.
     */
    static void processEvents();

    /** Callback for when get a down touch event. To be called by Wayland code.
     * Simply creates the raw event and adds it to the event queue. Important
     * to not block processing here.
     */
    static void downEventCallback(int serial, int time, int id, int x, int y) {
        storePosition(id, math::Vec(x, y));

        addEventToQueue(
            WaylandTouchEvent(serial, WaylandTouchEvent::TOUCH_DOWN, id, x, y));
    }

    /** Callback for when get an up touch event. To be called by Wayland code.
     * Simply creates the raw event and adds it to the event queue. Important
     * to not block processing here. Uses last known position since up events
     * don't provide position info.
     */
    static void upEventCallback(int serial, int time, int id) {
        math::Vec pos = getLastPosition(id);
        addEventToQueue(WaylandTouchEvent(serial, WaylandTouchEvent::TOUCH_UP,
                                          id, pos.getX(), pos.getY()));
    }

    /** Callback for when get a motion touch event. To be called by Wayland
     * code. Simply creates the raw event and adds it to the event queue.
     * Important to not block processing here.
     */
    static void motionEventCallback(int time, int id, int x, int y) {
        storePosition(id, math::Vec(x, y));

        addEventToQueue(
            WaylandTouchEvent(0, WaylandTouchEvent::TOUCH_MOTION, id, x, y));
    }

    /** Callback for when get a shape touch event. To be called by Wayland
     * code. Simply creates the raw event and adds it to the event queue.
     * Important to not block processing here.
     */
    static void shapeEventCallback(int id, float major, float minor) {
        addEventToQueue(WaylandShapeEvent(id, major, minor));
    }

    /** Callback for when get an orientation touch event. To be called by
     * Wayland code. Simply creates the raw event and adds it to the event
     * queue. Important to not block processing here.
     */
    static void orientationEventCallback(int id, float orientation) {
        addEventToQueue(WaylandOrientationEvent(id, orientation));
    }

    /** Callback for when get a frame touch event. To be called by Wayland
     * code. Currently does nothing so touch events are aggregated instead by
     * processEvents().
     */
    static void frameEventCallback() {}

    /** Adds the given touch event to the appropriate event queue. */
    static void addEventToQueue(const WaylandTouchEvent& event);

   private:
    /** Stores the last known position for the given touch id. */
    static void storePosition(int id, const math::Vec& pos);

    /** Returns the last known position for the given touch id. */
    static math::Vec getLastPosition(int id);

   private:
    static const int NUM_TOUCHPOINTS = 10;
    static std::queue<WaylandTouchEvent> eventQueues_[NUM_TOUCHPOINTS];

    // For tracking last touch position for each touch point id. Needed since
    // release events don't provide position, yet position is needed by event
    // handers.
    static math::Vec lastTouchPos_[NUM_TOUCHPOINTS];
};


}  // namespace window
}  // namespace rack