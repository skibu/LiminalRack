#pragma once

#include <widget/Widget.hpp>

#include <list>

namespace rack {
namespace widget {

// An alias for function ptr that handles a ActionEvent for a Widget
using ActionEventHandlerPtr =
        void (Widget::*)(const Widget::ActionEvent& event);

/** For handling mouse or touch events that need to be delayed in order to
 * first determine if superceded by another event that takes time to be
 * recognized. Examples are distinguishing single-click from double-click, or
 * long-press from normal press.
 */
struct EventDelayInfo { 
    // When to execute the event
    float timeWhenShouldExecute;

    // Copy of the event to be executed.
    Widget::ActionEvent event;

    // The widget that will receive the event
    Widget* eventReceiver;

    // Declare a pointer to a member function of the Base class that takes no
    // arguments and returns void.
    ActionEventHandlerPtr eventHandlerPtr;
};

class EventDelayer {
    public:
    /** Adds a delayed event to be processed later.
     * @param delaySeconds Number of seconds to delay the event.
     * @param event The event to be processed later. Copy is made.
     * @param eventReceiver The widget that will receive the event.
     * @param eventHandlerPtr Pointer to the member function of Widget that
     * will handle the event.
     */
    void addDelayedEvent(
        float delaySeconds, const Widget::ActionEvent& event, Widget* eventReceiver,
        ActionEventHandlerPtr eventHandlerPtr);

    /** Removes any delayed events for the given event receiver widget. */
    void removeDelayedEvent(Widget* eventReceiver);

    /** Processes any delayed events whose time has come. To be called from the
     * main event loop. */
    void processDelayedEvents();

   private:
    // Set of delayed events to be processed later
    std::list<EventDelayInfo> delayedEvents_;
};

} // namespace widget
} // namespace rack