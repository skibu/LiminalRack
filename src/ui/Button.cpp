#include <ui/Button.hpp>
#include <context.hpp>
#include <settings.hpp>

namespace rack {
namespace ui {

/** Default constructor needs to be declared here instead of inlined
 * so that modules that use it, like 4ms, can link against it.
 * Sets the button height to settings::bndWidgetHeight. If you
 * are using a different font size you might want to set the height
 * to something else. Uses blendish bndToolButton() to actually draw 
 * the button.
 */
Button::Button() {
    setHeight(settings::bndWidgetHeight);
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

void Button::onButton(const ButtonEvent& event) {
    DEBUG("Button.onButton() called widget=%s button=%d action=%d x=%.1f y=%.1f",
          getName().c_str(), event.button, event.action, event.pos.getX(),
          event.pos.getY());

    // Handle button press/release
    if (event.button == GLFW_MOUSE_BUTTON_LEFT) {
        if (event.action == GLFW_PRESS) {
            if (quantity_)
                quantity_->setMax();
        } else if (event.action == GLFW_RELEASE) {
            if (quantity_)
                quantity_->setMin();

            // Dispatch Action event
            DEBUG("Button.onButton() called and calling onAction() for widget=%s ",
                  getName().c_str());
            ActionEvent e;
            onAction(e);
        }

        // Consume if not consumed by child
        if (!event.isConsumed())
            event.consume(this);
    }
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
