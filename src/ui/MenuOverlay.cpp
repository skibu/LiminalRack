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
	// Adopt parent's size
	setBox(getParent()->getBox().zeroPos());

	Widget::step();
}


void MenuOverlay::onButton(const ButtonEvent& e) {
    DEBUG("MenuOverlay::onButton() called");

	OpaqueWidget::onButton(e);
	if (e.isConsumed() && e.getTarget() != this)
		return;

	if (e.action == GLFW_PRESS) {
		ActionEvent eAction;
		onAction(eAction);
	}

	// Consume all buttons.
	e.consume(this);
}


void MenuOverlay::onHoverKey(const HoverKeyEvent& e) {
	OpaqueWidget::onHoverKey(e);
	if (e.isConsumed())
		return;

	if (e.action == GLFW_PRESS && e.isKeyCommand(GLFW_KEY_ESCAPE)) {
		ActionEvent eAction;
		onAction(eAction);
	}

	// Consume all keys.
	// Unfortunately this prevents MIDI computer keyboard from playing while a menu is open, but that might be a good thing for safety.
	e.consume(this);
}


void MenuOverlay::onAction(const ActionEvent& e) {
    DEBUG("MenuOverlay::onAction() called, so requesting delete");
    // Close the menu overlay, which closes the menus within it
	requestDelete();
}


} // namespace ui
} // namespace rack
