#include <settings.hpp>
#include <ui/MenuSeparator.hpp>

namespace rack {
namespace ui {


MenuSeparator::MenuSeparator() {
	// Set the size of the separator. It can be much smaller than other menu items.
    box.size.y = rack::settings::bndWidgetHeight / 4;
}

void MenuSeparator::draw(const DrawArgs& args) {
	// Draw a horizontal line at the top of this widget area
	nvgBeginPath(args.vg);
	const float horizontal_indent = 8.0;
	float stroke_width = 3.0;
	float y = stroke_width / 2.0;
	nvgMoveTo(args.vg, horizontal_indent, y);
	nvgLineTo(args.vg, box.size.x - horizontal_indent, y);
	nvgStrokeWidth(args.vg, stroke_width);
	nvgStrokeColor(args.vg, color::alpha(bndGetTheme()->menuTheme.textColor, 0.25));
	nvgStroke(args.vg);
}


} // namespace ui
} // namespace rack
