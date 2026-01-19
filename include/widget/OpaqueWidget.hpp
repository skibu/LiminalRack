#pragma once
#include <widget/Widget.hpp>


namespace rack {
namespace widget {


/** A Widget that stops propagation of all recursive PositionEvents (such as ButtonEvent) but gives a chance for children to consume first.
Also consumes HoverEvent and ButtonEvent for left-clicks.
*/
struct OpaqueWidget : Widget {
    /** Constructor. Stores name of widget. Can't just use a default value for
     * name param since the original published SDK has a constructor with no
     * parameters. Therefore we provide both constructors.
     */
    OpaqueWidget(const std::string& name) : Widget(name) {}

    /** Default constructor. Uses empty string for name.
     */
    OpaqueWidget() : OpaqueWidget("") {}

	void onHover(const HoverEvent& event) override {
		Widget::onHover(event);
		event.stopPropagating();
		// Consume if not consumed by child
		if (!event.isConsumed())
			event.consume(this);
	}

    /** Called for a left click (GLFW_MOUSE_BUTTON_LEFT) if the current widget
     * doesn't have an override function. */
    void onButton(const ButtonEvent& event) override {
        DEBUG("onButton() called widget=%s button=%d action=%d x=%.1f y=%.1f",
              getName().c_str(), event.button, event.action, event.pos.getX(),
              event.pos.getY());

        // Recurse through children first
        Widget::onButton(event);

        // Mark the cloned event as done propagating through the branch
        event.stopPropagating();
		if (event.button == GLFW_MOUSE_BUTTON_LEFT) {
			// Consume if not consumed by child
			if (!event.isConsumed())
				event.consume(this);
		}
    }

    void onHoverKey(const HoverKeyEvent& event) override {
		Widget::onHoverKey(event);
		event.stopPropagating();
	}

	void onHoverText(const HoverTextEvent& event) override {
        DEBUG("onHoverText called");
		Widget::onHoverText(event);
		event.stopPropagating();
	}

	void onHoverScroll(const HoverScrollEvent& event) override {
        DEBUG("onHoverScroll called");
		Widget::onHoverScroll(event);
		event.stopPropagating();
	}

	void onDragHover(const DragHoverEvent& event) override {
		Widget::onDragHover(event);
		event.stopPropagating();
		// Consume if not consumed by child
		if (!event.isConsumed())
			event.consume(this);
	}

	void onPathDrop(const PathDropEvent& event) override {
        DEBUG("OpaqueWidget::onPathDrop called");
		Widget::onPathDrop(event);
		event.stopPropagating();
	}
};


} // namespace widget
} // namespace rack
