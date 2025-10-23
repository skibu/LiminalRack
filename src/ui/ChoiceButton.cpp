#include <ui/ChoiceButton.hpp>
#include <context.hpp>


namespace rack {
namespace ui {

void ChoiceButton::draw(const DrawArgs& args) {
    BNDwidgetState state = BND_DEFAULT;
    if (getEvent()->getHoveredWidget() == this) state = BND_HOVER;
    if (getEvent()->getDraggedWidget() == this) state = BND_ACTIVE;

    std::string text = this->text;
    if (text.empty() && getQuantity()) text = getQuantity()->getLabel();
    bndChoiceButton(args.vg, 0.0, 0.0, getWidth(), getHeight(), BND_CORNER_NONE,
                    state, -1, text.c_str());
}

} // namespace ui
} // namespace rack
