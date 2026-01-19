#include <ui/MenuOverlay.hpp>


namespace rack {
namespace ui {


MenuOverlay::MenuOverlay() {
	bgColor_ = nvgRGBA(0, 0, 0, 0);
}

MenuOverlay::~MenuOverlay() {
    TRACE("~MenuOverlay() called for widget %s so deleting children", getName().c_str());

    // Delete all child widgets (menus, etc.) within the MenuOverlay.
    // This is necessary since they are allocated on the heap.
    for (Widget* child : getChildren()) {
        child->requestDelete();
    }
}


void MenuOverlay::draw(const DrawArgs& args) {
	if (bgColor_.a > 0.f) {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, VEC_ARGS(getSize()));
		nvgFillColor(args.vg, bgColor_);
		nvgFill(args.vg);
	}

	OpaqueWidget::draw(args);
}


void MenuOverlay::step() {
    // Adopt parent's size. This is done here in step() since in constructor the
    // parent isn't yet set because parent.addChild() not called until after the
    // object has been constructed.
    setBox(getParent()->getBox().zeroPos());

    Widget::step();
}


void MenuOverlay::onButton(const ButtonEvent& event) {
    DEBUG(
        "onButton() called for MenuOverlay widget %s action=%d button=%d "
        "x=%.1f y=%.1f",
        getName().c_str(), event.action, event.button, event.pos.getX(), event.pos.getY());

    // See if child widgets want to handle the button event first
    OpaqueWidget::onButton(event);
	if (event.isConsumed() && event.getTarget() != this)
		return; // Child handled the event

	if (event.action == GLFW_PRESS) {
		ActionEvent eAction;
		onAction(eAction);
	}

	// Consume all buttons. Since MenuOverlay covers everything, this way
    // a click outside the menu will close it.
	event.consume(this);
}


void MenuOverlay::onHoverKey(const HoverKeyEvent& event) {
	OpaqueWidget::onHoverKey(event);
	if (event.isConsumed())
		return;

	if (event.action == GLFW_PRESS && event.isKeyCommand(GLFW_KEY_ESCAPE)) {
		ActionEvent eAction;
		onAction(eAction);
	}

    // Consume all keys.
    // Unfortunately this prevents MIDI computer keyboard from playing while a
    // menu is open, but that might be a good thing for safety.
    event.consume(this);
}


void MenuOverlay::onAction(const ActionEvent& event) {
    DEBUG("MenuOverlay::onAction() called, so requesting delete");
    // Close the menu overlay, which closes the menus within it
	requestDelete();
}


} // namespace ui
} // namespace rack
