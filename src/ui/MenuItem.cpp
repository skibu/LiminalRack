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

    if (getEvent()->hoveredWidget == this) state = BND_HOVER;

    // Set active state if this MenuItem is the Menu's active entry
    Menu* parentMenu = dynamic_cast<Menu*>(getParent());
    if (parentMenu && parentMenu->activeEntry == this) state = BND_ACTIVE;

    // Want to center text vertically. At first thought that needed a
    // y_centering_offset
    // to adjust the text position, but then found that a value of 0 works best.
    float y_centering_offset = 0;

    // Draw main text and background
    const BNDtheme* theme = bndGetTheme();
    if (!disabled) {
        // Draw label as active.
        // From bndMenuItem() implementation, draw background box
        if (state != BND_DEFAULT) {
            // Hightlight since it is active or hovered
            bndInnerBox(vg, 0.0, -1.0, getWidth(), getHeight() - 2.0, 0, 0, 0,
                        0,
                        bndOffsetColor(theme->menuItemTheme.innerSelectedColor,
                                       theme->menuItemTheme.shadeTop),
                        bndOffsetColor(theme->menuItemTheme.innerSelectedColor,
                                       theme->menuItemTheme.shadeDown));
            state = BND_ACTIVE;
        }
        // Draw the label as active and centered vertically in the box
        bndIconLabelValue(vg, x_offset, y_centering_offset,
                          getWidth() - x_offset, getHeight(), -1,
                          bndTextColor(&theme->menuItemTheme, state), BND_LEFT,
                          rack::settings::bndLabelFontSize, text.c_str(), NULL);
    } else {
        // Feature currently disabled, so draw label as inactive by drawing
        // dimmer text
        bndIconLabelValue(vg, x_offset, y_centering_offset, getWidth(),
                          getHeight(), -1, theme->menuTheme.textColor, BND_LEFT,
                          rack::settings::bndLabelFontSize, text.c_str(), NULL);
    }

    // Draw the text on the right of the menu item. Typically used for keyboard
    // shortcuts.
    float x = getWidth() - bndLabelWidth(vg, -1, rightText.c_str());
    NVGcolor rightColor = (state == BND_DEFAULT && !disabled)
                              ? bndGetTheme()->menuTheme.textColor
                              : bndGetTheme()->menuTheme.textSelectedColor;
    bndIconLabelValue(vg, x, y_centering_offset, getWidth(), getHeight(), -1,
                      rightColor, BND_LEFT, rack::settings::bndLabelFontSize,
                      rightText.c_str(), NULL);
}

void MenuItem::step() {
	// HACK use getWindow()->vg from the window.
	// All this does is inspect the font, so it shouldn't modify getWindow()->vg and should work when called from a widget::FramebufferWidget for example.
	setWidth(bndLabelWidth(getWindow()->vg_, -1, text.c_str()));
	if (!rightText.empty())
		setWidth(getWidth() + bndLabelWidth(getWindow()->vg_, -1, rightText.c_str()) - 10.0);
	// Add 10 more pixels because measurements on high-DPI screens are sometimes too small for some reason
	setWidth(getWidth() + 10.0);

	Widget::step();
}

void MenuItem::onEnter(const EnterEvent& e) {
	Menu* parentMenu = dynamic_cast<Menu*>(getParent());
	if (!parentMenu)
		return;

	parentMenu->activeEntry = NULL;

	// Try to create child menu
	Menu* childMenu = createChildMenu();
	if (childMenu) {
		parentMenu->activeEntry = this;
		childMenu->setPos(parentMenu->getPos().plus(getBox().getTopRight()));
	}
	parentMenu->setChildMenu(childMenu);
}

void MenuItem::onDragDrop(const DragDropEvent& e) {
	if (e.origin == this && !disabled) {
		int mods = getWindow()->getMods();
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
	nvgCircle(args.vg, 8.0 + radius, getHeight() / 2, radius);
	nvgFillColor(args.vg, color);
	nvgFill(args.vg);
	nvgStrokeWidth(args.vg, 1.0);
	nvgStrokeColor(args.vg, color::mult(color, 0.5));
	nvgStroke(args.vg);
}

void ColorDotMenuItem::step() {
	MenuItem::step();
	setWidth(getWidth() + 20.0);
}


} // namespace ui
} // namespace rack
