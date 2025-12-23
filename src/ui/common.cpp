/**
 * For managing colors of the various UI components. 
 */

#include <ui/common.hpp>
#include <settings.hpp>
#include <context.hpp>


namespace rack {
namespace ui {


void init() {
    INFO("Initializing low-level UI");

	settings::initBlendish();

    // Set the initial theme: light, dark, or hcdark
	refreshTheme();
}

void destroy() {
}

/** Returns a copy of a BNDwidgetTheme object. */
static BNDwidgetTheme createTheme(NVGcolor bg, NVGcolor fg) {
	BNDwidgetTheme w;
	w.outlineColor = color::lerp(bg, fg, 0.35);
	w.itemColor = fg;
	w.innerColor = color::lerp(bg, fg, 0.1);
	w.innerSelectedColor = color::lerp(bg, fg, 0.4);
	w.textColor = fg;
	w.textSelectedColor = fg;
	w.shadeTop = 0;
	w.shadeDown = 0;
	return w;
}

void setTheme(NVGcolor bg, NVGcolor fg) {
    // Create the overall theme
	BNDwidgetTheme w = createTheme(bg, fg);

	BNDtheme t;
	t.backgroundColor = bg;
	t.regularTheme = w;
	t.toolTheme = w;
	t.radioTheme = w;
	t.textFieldTheme = w;
	t.optionTheme = w;
	t.choiceTheme = w;
	t.numberFieldTheme = w;
	t.sliderTheme = w;
	t.scrollBarTheme = w;
	t.menuTheme = w;
	t.menuItemTheme = w;

    // Create the tooltip theme and store it
    t.tooltipTheme = createTheme(settings::tooltipBg, settings::tooltipFg);

	// Slider filled background
	t.sliderTheme.itemColor = color::lerp(bg, fg, 0.4);
	// Slider background
	t.sliderTheme.innerColor = color::lerp(bg, fg, 0.0);
	t.sliderTheme.innerSelectedColor = color::lerp(bg, fg, 0.1);

	// Text field background. Make it lighter than normal background
    // so that user can easily tell it apart from other widgets
	t.textFieldTheme.innerColor = color::lerp(bg, fg, 0.7);
	t.textFieldTheme.innerSelectedColor = color::lerp(bg, fg, 0.8);
	// Text
	t.textFieldTheme.textColor = color::lerp(bg, fg, 0.0);
	t.textFieldTheme.textSelectedColor = t.textFieldTheme.textColor;
	// Highlight selectedtext background
	t.textFieldTheme.itemColor = color::lerp(bg, fg, 0.5);

    // Scrollbar handle color
	t.scrollBarTheme.itemColor = color::lerp(bg, fg, 0.7);
    // Scrollbar handle alpha. Should be reasonably visible but still want
    // to be able to see content behind it.
    t.scrollBarTheme.itemColor.a = 0.8f;
    // Scrollbar track background color and alpha
	t.scrollBarTheme.innerColor = color::lerp(bg, fg, 0.1);
    t.scrollBarTheme.innerColor.a = 0.3f;

	// Menu background
	t.menuTheme.innerColor = bg;
	// Menu label text
	t.menuTheme.textColor = color::lerp(bg, fg, 0.6);
	t.menuTheme.textSelectedColor = t.menuTheme.textColor;

	bndSetTheme(t);
}


void refreshTheme() {
	if (settings::uiTheme == "light") {
		// Set main forground and background colors for light theme
		setTheme(settings::lightModeThemeBg, settings::lightModeThemeFg);
	}
	else if (settings::uiTheme == "hcdark") {
		// Set main forground and background colors for high contrast dark theme
		setTheme(nvgRGB(0x00, 0x00, 0x00), nvgRGB(0xff, 0xff, 0xff));
	}
	else {
		// Set main forground and background colors for dark theme
		setTheme(settings::darkModeThemeBg, settings::darkModeThemeFg);
	}
}


} // namespace ui
} // namespace rack
