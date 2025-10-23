#include <ui/Menu.hpp>


namespace rack {
namespace ui {


Menu::Menu() {
	setSize(0, 0);
}

Menu::~Menu() {
	setChildMenu(NULL);
}

void Menu::setChildMenu(Menu* menu) {
	if (childMenu) {
		childMenu->getParent()->removeChild(childMenu);
		delete childMenu;
		childMenu = NULL;
	}

	if (menu) {
		childMenu = menu;
		assert(getParent());
		getParent()->addChild(childMenu);
	}
}

void Menu::step() {
	Widget::step();

	// Set positions of children
	setSize(0, 0);
	for (widget::Widget* child : getChildren()) {
		if (!child->isVisible())
			continue;
		// Increment height, set position of child
		child->setPos(math::Vec(0, getHeight()));
		setHeight(getHeight() + child->getHeight());
        
		// Increase width based on maximum width of child
		if (child->getWidth() > getWidth()) {
			setWidth(child->getWidth());
		}
	}

	// Set widths of all children to maximum width
	for (widget::Widget* child : getChildren()) {
		child->setWidth(getWidth());
	}

	// Fit inside parent
	assert(getParent());
	setBox(getBox().nudge(getParent()->getBox().zeroPos()));
}

void Menu::draw(const DrawArgs& args) {
	bndMenuBackground(args.vg, 0.0, 0.0, getWidth(), getHeight(), cornerFlags);
	Widget::draw(args);
}

void Menu::onHoverScroll(const HoverScrollEvent& e) {
	if (getParent() && !getParent()->getBox().contains(getBox()))
		setY(getY() + e.scrollDelta.getY());
}


} // namespace ui
} // namespace rack
