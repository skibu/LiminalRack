#include <ui/List.hpp>


namespace rack {
namespace ui {

void List::step() {
    Widget::step();

    // Set positions of children
    setHeight(0.0);
    for (widget::Widget* child : getChildren()) {
        if (!child->isVisible()) continue;

        // Set position of child
        child->setPos(math::Vec(0.0, getHeight()));

        // Increment height
        setHeight(getHeight() + child->getHeight());
        // Resize width of child
        child->setWidth(getWidth());
    }
}

} // namespace ui
} // namespace rack
