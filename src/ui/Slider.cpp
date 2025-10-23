#include <settings.hpp>
#include <ui/Menu.hpp>
#include <ui/Slider.hpp>

namespace rack {
namespace ui {


static const float SENSITIVITY = 0.001f;


Slider::Slider() {
    setHeight(settings::bndWidgetHeight);
}

void Slider::draw(const DrawArgs& args) {
	BNDwidgetState state = BND_DEFAULT;
	if (getEvent()->hoveredWidget == this)
		state = BND_HOVER;
	if (getEvent()->draggedWidget == this)
		state = BND_ACTIVE;

	float progress = quantity ? quantity->getScaledValue() : 0.f;
	std::string text = quantity ? quantity->getString() : "";

	// If parent is a Menu, make corners sharp
	ui::Menu* parentMenu = dynamic_cast<ui::Menu*>(getParent());
	int flags = parentMenu ? BND_CORNER_ALL : BND_CORNER_NONE;
	bndSlider(args.vg, 0.0, 0.0, getWidth(), getHeight() - 4.0, flags, state, progress, text.c_str(), NULL);
}

void Slider::onDragStart(const DragStartEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	getWindow()->cursorLock();
}

void Slider::onDragMove(const DragMoveEvent& e) {
    if (quantity == nullptr) return;

    quantity->moveScaledValue(SENSITIVITY * e.mouseDelta.getX());
}

void Slider::onDragEnd(const DragEndEvent& e) {
	getWindow()->cursorUnlock();
}

void Slider::onDoubleClick(const DoubleClickEvent& e) {
    if (quantity == nullptr) return;

    quantity->reset();
}

} // namespace ui
} // namespace rack
