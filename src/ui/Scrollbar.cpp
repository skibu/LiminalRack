#include <ui/Scrollbar.hpp>
#include <ui/ScrollWidget.hpp>
#include <context.hpp>
#include <window/Window.hpp>
#include <settings.hpp>


namespace rack {
namespace ui {

const NVGcolor NULL_COLOR = color::BLACK_TRANSPARENT;

/** Internal data structure for Scrollbar to hide implementation details. */
struct Scrollbar::Internal {
    // Colors for the scrollbars
	NVGcolor track_color = NULL_COLOR;
	NVGcolor handle_color = NULL_COLOR;
};


Scrollbar::Scrollbar() {
    internal_ = new Internal;
    if (settings::hasTouchscreen) {
        // For touch screens use wider scrollbar handles so easier to touch
        setSize(BND_SCROLLBAR_WIDTH * 1.4, BND_SCROLLBAR_HEIGHT * 1.4);
    } else {
        setSize(BND_SCROLLBAR_WIDTH, BND_SCROLLBAR_HEIGHT);
    }
}


Scrollbar::~Scrollbar() {
    delete internal_;
}

void Scrollbar::setScrollbarColors(NVGcolor track_color,
                                   NVGcolor handle_color) {
  internal_->track_color = track_color;
  internal_->handle_color = handle_color;
}

void Scrollbar::draw(const DrawArgs& args) {
    ScrollWidget* sw = dynamic_cast<ScrollWidget*>(getParent());
    assert(sw);

    BNDwidgetState state = BND_DEFAULT;
    if (getEvent()->getHoveredWidget() == this) state = BND_HOVER;
    if (getEvent()->getDraggedWidget() == this) state = BND_ACTIVE;

    // Draw the scrollbar
    float handleOffset = sw->getHandleOffset()[vertical];
    float handleSize = sw->getHandleSize()[vertical];

    // Draw the scrollbar using blendish
    if (color::isEqual(internal_->track_color, NULL_COLOR) &&
        color::isEqual(internal_->handle_color, NULL_COLOR)) {
      // Use default colors
      bndScrollBar(args.vg, 0.0, 0.0, getWidth(), getHeight(), state,
                   handleOffset, handleSize);
    } else {
      // Use custom colors
      bndColoredScrollBar(args.vg, 0.0, 0.0, getWidth(), getHeight(), state,
                          handleOffset, handleSize, internal_->track_color,
                          internal_->handle_color);
    }
}

void Scrollbar::onButton(const ButtonEvent& e) {
    if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
        ScrollWidget* sw = dynamic_cast<ScrollWidget*>(getParent());
        assert(sw);

        float pos = e.pos[vertical];
        pos /= getSize()[vertical];
        float handleOffset = sw->getHandleOffset()[vertical];
        float handleSize = sw->getHandleSize()[vertical];
        float handlePos = math::rescale(
            handleOffset, 0.f, 1.f, handleSize / 2.f, 1.f - handleSize / 2.f);

        // Check if user clicked on handle
        if (std::fabs(pos - handlePos) > handleSize / 2.f) {
            // Jump to absolute position of the handle
            float offset = math::rescale(pos, handleSize / 2.f,
                                         1.f - handleSize / 2.f, 0.f, 1.f);
            sw->offset[vertical] =
                sw->containerBox.getPos()[vertical] +
                offset * (sw->containerBox.getSize()[vertical] -
                          sw->getSize()[vertical]);
        }
    }
    OpaqueWidget::onButton(e);
}

void Scrollbar::onDragStart(const DragStartEvent& e) {
}


void Scrollbar::onDragEnd(const DragEndEvent& e) {
}


void Scrollbar::onDragMove(const DragMoveEvent& e) {
	ScrollWidget* sw = dynamic_cast<ScrollWidget*>(getParent());
	assert(sw);

	// Move handle absolutely.
	float mouseDelta = e.mouseDelta[vertical];
	mouseDelta /= getAbsoluteZoom();

	float handleSize = sw->getHandleSize()[vertical];
	float handleBound = (1.f - handleSize) * getHeight();
	float offsetBound = sw->getContainerOffsetBound().getSize()[vertical];
	float offsetDelta = mouseDelta * offsetBound / handleBound;
	sw->offset[vertical] += offsetDelta;
}


} // namespace ui
} // namespace rack
