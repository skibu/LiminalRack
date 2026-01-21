#include <ui/RadioButton.hpp>
#include <context.hpp>


namespace rack {
namespace ui {


void RadioButton::draw(const DrawArgs& args) {
	BNDwidgetState state = BND_DEFAULT;
	if (getEvent()->getHoveredWidget() == this)
		state = BND_HOVER;

	if (quantity_) {
		if (quantity_->isMax())
			state = BND_ACTIVE;
	}

	std::string text = this->text_;
	if (text.empty() && quantity_)
		text = quantity_->getLabel();
	bndRadioButton(args.vg, 0.0, 0.0, getWidth(), getHeight(), BND_CORNER_NONE, state, -1, text.c_str());
}

/*
void RadioButton::onDragStart(const DragStartEvent& e) {
	OpaqueWidget::onDragStart(e);
}


void RadioButton::onDragEnd(const DragEndEvent& e) {
	OpaqueWidget::onDragEnd(e);
}
*/


void RadioButton::onDragDrop(const DragDropEvent& e) {
    DEBUG("deprecated RadioButton.onDragDrop() called for RadioButton %s", getName().c_str());
    /*
	if (e.origin == this) {
		if (quantity_)
			quantity_->toggle();

		ActionEvent eAction;
		onAction(eAction);
	} 
    */
}


void RadioButton::onButton(const ButtonEvent& event) {
    // On left clicks toggle value and call onAction() event
    if (event.button == GLFW_MOUSE_BUTTON_LEFT && event.action == GLFW_PRESS) {
        DEBUG("RadioButton.onButton() called for RadioButton %s button=%d %s action=%d %s", 
              getName().c_str(), event.button, event.button ? "LEFT" : "RIGHT",
              event.action, event.action ? "PRESS" : "RELEASE");

        if (quantity_) quantity_->toggle();

        ActionEvent eAction;
        onAction(eAction);
    }

    // If left click or release consume any uncomsumed event
    if (event.button == GLFW_MOUSE_BUTTON_LEFT) {
        // Consume if not consumed by child
        if (!event.isConsumed()) event.consume(this);
    }
}

} // namespace ui
} // namespace rack
