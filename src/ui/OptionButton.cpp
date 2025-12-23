#include <ui/OptionButton.hpp>


namespace rack {
namespace ui {


void OptionButton::draw(const DrawArgs& args) {
	BNDwidgetState state = BND_DEFAULT;
	if (quantity_) {
		if (quantity_->isMax())
			state = BND_ACTIVE;
	}

	std::string text = this->text_;
	if (text.empty() && quantity_)
		text = quantity_->getLabel();
	bndOptionButton(args.vg, 0.0, 0.0, INFINITY, getHeight(), state, text.c_str());
}


} // namespace ui
} // namespace rack
