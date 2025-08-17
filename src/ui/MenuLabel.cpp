#include <context.hpp>
#include <settings.hpp>
#include <ui/MenuLabel.hpp>

namespace rack {
namespace ui {


void MenuLabel::draw(const DrawArgs& args) {
    float y_centering_offset = (settings::bndWidgetHeight - settings::bndLabelFontSize) / 2.0;
    bndMenuLabel(args.vg, 0.0, -y_centering_offset, box.size.x, box.size.y, -1, text.c_str());
}

void MenuLabel::step() {
	// Add 10 more pixels because Retina measurements are sometimes too small
	const float rightPadding = 10.0;
	// HACK use APP->window->vg from the window.
	box.size.x = bndLabelWidth(APP->window->vg, -1, text.c_str()) + rightPadding;
	Widget::step();
}


} // namespace ui
} // namespace rack
