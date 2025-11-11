#include <context.hpp>
#include <settings.hpp>
#include <ui/MenuLabel.hpp>
#include <color.hpp>

namespace rack {
namespace ui {

void MenuLabel::draw(const DrawArgs& args) {
    float y_centering_offset =
        (settings::bndWidgetHeight - settings::bndLabelFontSize) / 2.0;

    if (settings::isNotVCVRack) {
        // For non-VCV Rack apps nice to enhance UI by drawing a different
        // background color for non-clickable labels. This way can more easily
        // tell the different between non-clickable labels and inactive buttons
        // that will still have the themes regular background color.

        // Determine background color to use. For now make it darker than the
        // usual theme color.
        const BNDtheme* theme = bndGetTheme();
        NVGcolor theme_bg = theme->menuItemTheme.innerColor;
        NVGcolor bg_color =
            color::brightness(theme_bg) > 0.2
                ?
                // Theme bg is dark so make label background lighter
                color::plus(theme_bg, nvgRGB(45, 45, 45))
                :
                // Theme bg is reasonably light so make label background darker
                color::minus(theme_bg, nvgRGB(45, 45, 45));

        // Determine the box for drawing background of label
        float width = getWidth();
        // Could add a bit more height but nice to have labels be a bit smaller
        float height = getHeight() - 2.0;
        float x = 0.0;
        // Need to adjust a bit so that box is vertically centered
        float y = -y_centering_offset;

        // Draw the background box
        bndInnerBox(args.vg, x, y, width, height, 0.0, 0.0, 0.0, 0.0, bg_color,
                    bg_color);
    }

    // Draw the label, centered verticially
   bndMenuLabel(args.vg, 0.0, -y_centering_offset, getWidth(), getHeight(), -1,
                 text.c_str());
}

void MenuLabel::step() {
	// Add 10 more pixels because Retina measurements are sometimes too small
	const float rightPadding = 10.0;
	// HACK use getWindow()->vg from the window.
	setWidth(bndLabelWidth(getWindow()->vg_, -1, text.c_str()) + rightPadding);
	Widget::step();
}


} // namespace ui
} // namespace rack
