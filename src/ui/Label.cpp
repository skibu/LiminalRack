#include <ui/Label.hpp>
#include <settings.hpp>
namespace rack {
namespace ui {


Label::Label() {
	fontSize = rack::settings::bndLabelFontSize;
	box.size.y = fontSize + 12;
	lineHeight = 1.2;
	color = color::BLACK_TRANSPARENT;
}

void Label::draw(const DrawArgs& args) {
	// TODO
	// Custom font sizes do not work with right or center alignment
	float x;
	switch (alignment) {
		default:
		case LEFT_ALIGNMENT: {
			x = 0.0;
		} break;
		case RIGHT_ALIGNMENT: {
			x = box.size.x - bndLabelWidth(args.vg, -1, text.c_str());
		} break;
		case CENTER_ALIGNMENT: {
			x = (box.size.x - bndLabelWidth(args.vg, -1, text.c_str())) / 2.0;
		} break;
	}

	nvgTextLineHeight(args.vg, lineHeight);
	NVGcolor colorActual = (color.a > 0.f) ? color : bndGetTheme()->regularTheme.textColor;
	bndIconLabelValue(args.vg, x, 0.0, box.size.x, box.size.y, -1, colorActual, BND_LEFT, fontSize, text.c_str(), NULL);
}


} // namespace ui
} // namespace rack
