#include <ui/MenuItem.hpp>
#include <ui/MenuOverlay.hpp>
#include <settings.hpp>

namespace rack {
namespace ui {


void MenuItem::draw(const DrawArgs& args) {
	drawOffset(args.vg, 0);
}

void MenuItem::drawOffset(NVGcontext* vg, float x_offset) {
	BNDwidgetState state = BND_DEFAULT;

	if (APP->event->hoveredWidget == this)
		state = BND_HOVER;

	// Set active state if this MenuItem is the Menu's active entry
	Menu* parentMenu = dynamic_cast<Menu*>(parent);
	if (parentMenu && parentMenu->activeEntry == this)
		state = BND_ACTIVE;

	// Draw main text and background
	const BNDtheme* theme = bndGetTheme();
	if (!disabled) {
		// Draw label as active.
		// From bndMenuItem() implementation, draw background box
		if (state != BND_DEFAULT) {
			bndInnerBox(vg, 0.0, 0.0, box.size.x, box.size.y, 0, 0, 0, 0,
				bndOffsetColor(theme->menuItemTheme.innerSelectedColor, theme->menuItemTheme.shadeTop),
				bndOffsetColor(theme->menuItemTheme.innerSelectedColor, theme->menuItemTheme.shadeDown));
			state = BND_ACTIVE;
		}
		// Draw the label centered vertically in the box
		float y_centering_offset = (settings::bndWidgetHeight - settings::bndLabelFontSize) / 2.0;
		bndIconLabelValue(vg, x_offset + 0.0, -y_centering_offset, box.size.x - x_offset, box.size.y, -1,
			bndTextColor(&theme->menuItemTheme, state), BND_LEFT,
			rack::settings::bndLabelFontSize, text.c_str(), NULL);
	}
	else {
		// Draw label as inactive.
		// From bndMenuLabel() implementation
		bndIconLabelValue(vg, x_offset + 0.0, 0.0, box.size.x, box.size.y, -1,
							theme->menuTheme.textColor, BND_LEFT,
							rack::settings::bndLabelFontSize, text.c_str(), NULL);
    }

    // Draw the text on the right of the menu item. Typically used for keyboard shortcuts.
	float x = box.size.x - bndLabelWidth(vg, -1, rightText.c_str());
	NVGcolor rightColor = (state == BND_DEFAULT && !disabled)
								? bndGetTheme()->menuTheme.textColor
								: bndGetTheme()->menuTheme.textSelectedColor;
	bndIconLabelValue(vg, x, 0.0, box.size.x, box.size.y, -1, rightColor, BND_LEFT,
						rack::settings::bndLabelFontSize, rightText.c_str(), NULL);
}

void MenuItem::step() {
	// HACK use APP->window->vg from the window.
	// All this does is inspect the font, so it shouldn't modify APP->window->vg and should work when called from a widget::FramebufferWidget for example.
	box.size.x = bndLabelWidth(APP->window->vg, -1, text.c_str());
	if (!rightText.empty())
		box.size.x += bndLabelWidth(APP->window->vg, -1, rightText.c_str()) - 10.0;
	// Add 10 more pixels because measurements on high-DPI screens are sometimes too small for some reason
	box.size.x += 10.0;

	Widget::step();
}

void MenuItem::onEnter(const EnterEvent& e) {
	Menu* parentMenu = dynamic_cast<Menu*>(parent);
	if (!parentMenu)
		return;

	parentMenu->activeEntry = NULL;

	// Try to create child menu
	Menu* childMenu = createChildMenu();
	if (childMenu) {
		parentMenu->activeEntry = this;
		childMenu->box.pos = parent->box.pos.plus(box.getTopRight());
	}
	parentMenu->setChildMenu(childMenu);
}

void MenuItem::onDragDrop(const DragDropEvent& e) {
	if (e.origin == this && !disabled) {
		int mods = APP->window->getMods();
		doAction((mods & RACK_MOD_MASK) != RACK_MOD_CTRL);
	}
}

void MenuItem::doAction(bool consume) {
	widget::EventContext cAction;
	ActionEvent eAction;
	eAction.context = &cAction;
	if (consume) {
		eAction.consume(this);
	}
	onAction(eAction);
	if (!cAction.consumed)
		return;

	// Close menu
	MenuOverlay* overlay = getAncestorOfType<MenuOverlay>();
	if (overlay) {
		overlay->requestDelete();
	}
}


void MenuItem::onAction(const ActionEvent& e) {
}


void ColorDotMenuItem::draw(const DrawArgs& args) {
	drawOffset(args.vg, 20.0);

	// Color dot
	nvgBeginPath(args.vg);
	float radius = 6.0;
	nvgCircle(args.vg, 8.0 + radius, box.size.y / 2, radius);
	nvgFillColor(args.vg, color);
	nvgFill(args.vg);
	nvgStrokeWidth(args.vg, 1.0);
	nvgStrokeColor(args.vg, color::mult(color, 0.5));
	nvgStroke(args.vg);
}

void ColorDotMenuItem::step() {
	MenuItem::step();
	box.size.x += 20.0;
}


} // namespace ui
} // namespace rack
