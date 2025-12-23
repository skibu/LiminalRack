#include <ui/RadioButton.hpp>
#include <context.hpp>


namespace rack {
namespace ui {


void RadioButton::draw(const DrawArgs& args) {
	BNDwidgetState state = BND_DEFAULT;
	if (getEvent()->getHoveredWidget() == this)
		state = BND_HOVER;

	if (quantity_) {
		if (quantity_->isMax())
			state = BND_ACTIVE;
	}

	std::string text = this->text_;
	if (text.empty() && quantity_)
		text = quantity_->getLabel();
	bndRadioButton(args.vg, 0.0, 0.0, getWidth(), getHeight(), BND_CORNER_NONE, state, -1, text.c_str());
}


void RadioButton::onDragStart(const DragStartEvent& e) {
	OpaqueWidget::onDragStart(e);
}


void RadioButton::onDragEnd(const DragEndEvent& e) {
	OpaqueWidget::onDragEnd(e);
}


void RadioButton::onDragDrop(const DragDropEvent& e) {
	if (e.origin == this) {
		if (quantity_)
			quantity_->toggle();

		ActionEvent eAction;
		onAction(eAction);
	}
}


} // namespace ui
} // namespace rack
