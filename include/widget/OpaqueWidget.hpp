#pragma once
#include <widget/Widget.hpp>


namespace rack {
namespace widget {


/** A Widget that stops propagation of all recursive PositionEvents (such as ButtonEvent) but gives a chance for children to consume first.
Also consumes HoverEvent and ButtonEvent for left-clicks.
*/
struct OpaqueWidget : Widget {
    OpaqueWidget(const std::string& name = "") : Widget(name) {}

	void onHover(const HoverEvent& e) override {
		Widget::onHover(e);
		e.stopPropagating();
		// Consume if not consumed by child
		if (!e.isConsumed())
			e.consume(this);
	}

	void onButton(const ButtonEvent& e) override {
        DEBUG("OpaqueWidget::onButton called button=%d action=%d x=%.1f y=%.1f",
              e.button, e.action, e.pos.getX(), e.pos.getY());
        Widget::onButton(e);
        e.stopPropagating();
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			// Consume if not consumed by child
			if (!e.isConsumed())
				e.consume(this);
		}
	}

	void onHoverKey(const HoverKeyEvent& e) override {
		Widget::onHoverKey(e);
		e.stopPropagating();
	}

	void onHoverText(const HoverTextEvent& e) override {
        DEBUG("OpaqueWidget::onHoverText called");
		Widget::onHoverText(e);
		e.stopPropagating();
	}

	void onHoverScroll(const HoverScrollEvent& e) override {
        DEBUG("OpaqueWidget::onHoverScroll called");
		Widget::onHoverScroll(e);
		e.stopPropagating();
	}

	void onDragHover(const DragHoverEvent& e) override {
		Widget::onDragHover(e);
		e.stopPropagating();
		// Consume if not consumed by child
		if (!e.isConsumed())
			e.consume(this);
	}

	void onPathDrop(const PathDropEvent& e) override {
        DEBUG("OpaqueWidget::onPathDrop called");
		Widget::onPathDrop(e);
		e.stopPropagating();
	}
};


} // namespace widget
} // namespace rack
