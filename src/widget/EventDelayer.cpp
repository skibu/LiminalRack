#include "widget/EventDelayer.hpp"
#include <system.hpp>

namespace rack {
namespace widget {

void EventDelayer::addDelayedEvent(float delaySeconds,
                                   const Widget::ActionEvent& event,
                                   Widget* eventReceiver,
                                   ActionEventHandlerPtr eventHandlerPtr) {
    // Make sure parameters are valid
    assert(eventReceiver != nullptr);
    assert(eventHandlerPtr != nullptr);

    // Create the EventDelayInfo
    EventDelayInfo edi;
    edi.timeWhenShouldExecute = system::getTime() + delaySeconds;
    edi.event = event;
    edi.eventReceiver = eventReceiver;
    edi.eventHandlerPtr = eventHandlerPtr;

    // Add to the list of delayed events
    delayedEvents_.push_back(edi);
}

void EventDelayer::removeDelayedEvent(Widget* eventReceiver){
    // Iterate through the delayed events and                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               remove those matching the receiver
    for (auto it = delayedEvents_.begin(); it != delayedEvents_.end();) {
        EventDelayInfo& edi = *it;
        if (edi.eventReceiver == eventReceiver) {
            DEBUG("Removing delayed event for widget: %s",
                  edi.eventReceiver->getName().c_str());

            // Remove from the list
            it = delayedEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

void EventDelayer::processDelayedEvents() {
    double currentTime = system::getTime();

    // Iterate through the delayed events and execute those whose time has come
    for (auto it = delayedEvents_.begin(); it != delayedEvents_.end();) {
        EventDelayInfo& edi = *it;
        if (currentTime >= edi.timeWhenShouldExecute) {
            // Time to execute the event
            DEBUG("Processing delayed event for widget: %s",
                  edi.eventReceiver->getName().c_str());

            // Call the event handler on the receiver widget
            (edi.eventReceiver->*(edi.eventHandlerPtr))(edi.event);

            // Remove from the list
            it = delayedEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace widget
} // namespace rack                                   