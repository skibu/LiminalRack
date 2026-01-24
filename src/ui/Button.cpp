#include <ui/Button.hpp>
#include <context.hpp>
#include <settings.hpp>
#include <widget/EventDelayer.hpp>
#include <widget/event.hpp>
#include <widget/PimplAdder.hpp>

namespace rack {
namespace ui {

struct ButtonInternals {
    // Whether single-click action events are delayed to see if superseded by a
    // double-click. If true, then single clicks are delayed in order to first
    // see if part of a double-click. If false, then single clicks are
    // immediate. Default is false sincee double-clicks are relatively rare and
    // want single clicks to be responsive.
    bool doubleClickTakesPrecedence = false;
};

/** Default constructor needs to be declared here instead of inlined
 * so that modules that use it, like 4ms, can link against it.
 * Sets the button height to settings::bndWidgetHeight. If you
 * are using a different font size you might want to set the height
 * to something else. Uses blendish bndToolButton() to actually draw 
 * the button.
 */
Button::Button() {
    setHeight(settings::bndWidgetHeight);
    widget::PimplAdder<Button, ButtonInternals>::create(this);
}

Button::Button(const std::string& text, const std::string& name)
    : OpaqueWidget(name) {
    setText(text);

    // Do other initialization that default constructor does
    setHeight(settings::bndWidgetHeight);
    widget::PimplAdder<Button, ButtonInternals>::create(this);
}

Button::~Button() {
    // Clean up internal struct
    widget::PimplAdder<Button, ButtonInternals>::cleanup(this);
}

void Button::setDoubleClickTakesPrecedence(bool takePrecedence) {
    ButtonInternals* internals =
        widget::PimplAdder<Button, ButtonInternals>::get(this);
    internals->doubleClickTakesPrecedence = takePrecedence;
}

double Button::getDoubleClickTakesPrecedence() {
    ButtonInternals* internals =
        widget::PimplAdder<Button, ButtonInternals>::get(this);
    return internals->doubleClickTakesPrecedence;
}

void Button::draw(const DrawArgs& args) {
    BNDwidgetState state = BND_DEFAULT;
    if (getEvent()->getHoveredWidget() == this) state = BND_HOVER;
    if (getEvent()->getDraggedWidget() == this) state = BND_ACTIVE;

    std::string text = this->text_;
    if (text.empty() && quantity_) text = quantity_->getLabel();
    bndToolButton(args.vg, 0.0, 0.0, getWidth(), getHeight(), BND_CORNER_NONE,
                  state, -1, text.c_str());
}

void Button::triggerActionEvent(const ButtonEvent& buttonEvent) {
    // Create the ActionEvent to be sent
    ActionEvent actionEvent;
    widget::EventContext eventContext;
    actionEvent.context = &eventContext;

    // If button press then delay processing the event
    bool shouldDelayEventProcessing = getDoubleClickTakesPrecedence();
    if (shouldDelayEventProcessing && buttonEvent.action == GLFW_PRESS) {
        DEBUG("Button.onButton() delaying onAction processing for widget=%s ",
              getName().c_str());

        // Create a delay event that will processing the button event
        const float DELAY_SECS = settings::doubleClickMaxDuration;
        getEvent()->getEventDelayer()->addDelayedEvent(DELAY_SECS, actionEvent,
                                                       this, &Widget::onAction);

        return;
    }

    // Don't need to delay, so process the event now
    DEBUG("Button.onButton() called and calling onAction() for widget=%s ",
          getName().c_str());
    onAction(actionEvent);
}

void Button::onButton(const ButtonEvent& event) {
    DEBUG("Button.onButton() called widget=%s button=%d action=%d x=%.1f y=%.1f",
          getName().c_str(), event.button, event.action, event.pos.getX(),
          event.pos.getY());

    // Handle button press/release
    if (event.button == GLFW_MOUSE_BUTTON_LEFT) {
        if (event.action == GLFW_PRESS) {
            if (quantity_)
                quantity_->setMax();

            // Initiate the onAction event, either immediately or delayed. This
            // is done for a button press, instead of release, so that it occurs
            // as rapidly as possible, which is important for music.
            triggerActionEvent(event);
        } else if (event.action == GLFW_RELEASE) {
            if (quantity_)
                quantity_->setMin();
        }

        // Consume if not consumed by child
        if (!event.isConsumed())
            event.consume(this);
    }
}

void Button::onDoubleClick(const DoubleClickEvent& event) {
    DEBUG("Button.onDoubleClick() called for widget=%s", getName().c_str());

    // Remove any delayed action events since double-click supersedes them
    getEvent()->getEventDelayer()->removeDelayedEvent(this);
}

void Button::onAction(const ActionEvent& event) {
    DEBUG("Button.onAction() called for widget=%s", getName().c_str());

    // Trigger any action listeners
    Widget::onAction(event);
}
/*
void Button::onDragStart(const DragStartEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

    DEBUG("Button.onDragStart() called widget=%s button=%d", getName().c_str(),
          e.button);

	if (quantity_)
		quantity_->setMax();
}
*/

void Button::onDragEnd(const DragEndEvent& e) {
    DEBUG("deprecatedButton.onDragEnd() called widget=%s button=%d", getName().c_str(),
          e.button);
    /*
    if (quantity_)
		quantity_->setMin();
    */
}


void Button::onDragDrop(const DragDropEvent& e) {
    DEBUG("deprecated Button.onDragDrop() called widget=%s button=%d", getName().c_str(),
          e.button);

    /*
	if (e.origin == this) {
        DEBUG("Button.onDragDrop() called and calling onAction() for widget=%s ",
              getName().c_str());

        ActionEvent eAction;
		onAction(eAction);
	}
    */
}

} // namespace ui
} // namespace rack
