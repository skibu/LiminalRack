#include <ui/ScrollWidget.hpp>
#include <context.hpp>


namespace rack {
namespace ui {


const NVGcolor NULL_COLOR = color::BLACK_TRANSPARENT;

/** Internal data structure for ScrollWidget to hide implementation details. */
struct ScrollWidget::Internal {
	bool scrolling = false;

	// Colors for the main rectangle within the scrolled window
	NVGcolor bg_color = NULL_COLOR;
	NVGcolor outline_color = NULL_COLOR;
};


ScrollWidget::ScrollWidget() {
	internal_ = new Internal;

	container = new widget::Widget;
	addChild(container);

	horizontalScrollbar_ = new Scrollbar;
	horizontalScrollbar_->vertical = false;
	horizontalScrollbar_->hide();
	addChild(horizontalScrollbar_);

	verticalScrollbar_ = new Scrollbar;
	verticalScrollbar_->vertical = true;
	verticalScrollbar_->hide();
	addChild(verticalScrollbar_);
}


ScrollWidget::~ScrollWidget() {
	delete internal_;
}

void ScrollWidget::setColors(NVGcolor bg_color, NVGcolor outline_color) {
	internal_->bg_color = bg_color;
	internal_->outline_color = outline_color;
}

void ScrollWidget::setScrollbarColors(NVGcolor track_color,
									   NVGcolor handle_color) {
	verticalScrollbar_->setScrollbarColors(track_color, handle_color);
	horizontalScrollbar_->setScrollbarColors(track_color, handle_color);
}	

void ScrollWidget::scrollTo(math::Rect r) {
	math::Rect bound = math::Rect::fromMinMax(r.getBottomRight().minus(getSize()), r.getPos());
	offset = offset.clampSafe(bound);
}


math::Rect ScrollWidget::getContainerOffsetBound() {
	math::Rect r;
	r.setPos(containerBox.getPos());
	r.setSize(containerBox.getSize().minus(getSize()));
	return r;
}


math::Vec ScrollWidget::getHandleOffset() {
	return offset.minus(containerBox.getPos()).div(getContainerOffsetBound().getSize());
}


math::Vec ScrollWidget::getHandleSize() {
	return getSize().div(containerBox.getSize());
}


bool ScrollWidget::isScrolling() {
	return internal_->scrolling;
}


void ScrollWidget::draw(const DrawArgs& args) {
	nvgScissor(args.vg, RECT_ARGS(args.clipBox));

	// Draw rectangle that scrolls with a different background to differentiate it
	if (!color::isEqual(internal_->bg_color, color::BLACK_TRANSPARENT) ||
		!color::isEqual(internal_->outline_color, color::BLACK_TRANSPARENT)) {
		bndBackgroundColor(args.vg, 0.0, 0.0, getWidth(), getHeight(), 0,
						   internal_->bg_color, internal_->outline_color);
	}

	Widget::draw(args);
	nvgResetScissor(args.vg);
}


void ScrollWidget::step() {
	Widget::step();

	// Set containerBox cache
	containerBox = container->getVisibleChildrenBoundingBox();

	// Clamp scroll offset
	math::Rect offsetBounds = getContainerOffsetBound();
	offset = offset.clamp(offsetBounds);

	// Update the container's position from the offset
	container->setPos(offset.neg().round());

	// Make scrollbars visible only if there is a positive range to scroll.
	if (hideScrollbars_) {
		horizontalScrollbar_->setVisible(false);
		verticalScrollbar_->setVisible(false);
	}
	else {
		horizontalScrollbar_->setVisible(offsetBounds.getWidth() > 0.f);
		verticalScrollbar_->setVisible(offsetBounds.getHeight() > 0.f);
	}

	// Reposition and resize scroll bars
	math::Vec inner = getSize().minus(math::Vec(verticalScrollbar_->getWidth(), horizontalScrollbar_->getHeight()));
	horizontalScrollbar_->setY(inner.getY());
	verticalScrollbar_->setX(inner.getX());
	horizontalScrollbar_->setWidth(verticalScrollbar_->isVisible() ? inner.getX() : getWidth());
	verticalScrollbar_->setHeight(horizontalScrollbar_->isVisible() ? inner.getY() : getHeight());
}


void ScrollWidget::onHover(const HoverEvent& e) {
	OpaqueWidget::onHover(e);

	if (!e.mouseDelta.isZero()) {
		internal_->scrolling = false;
	}
}


void ScrollWidget::onButton(const ButtonEvent& e) {
	math::Rect offsetBound = getContainerOffsetBound();
	// Check if scrollable
	if (offsetBound.getWidth() > 0.f || offsetBound.getHeight() > 0.f) {
		// Handle Alt-click before children, since most widgets consume Alt-click without needing to.
		if (e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == GLFW_MOD_ALT) {
			e.consume(this);
			return;
		}
		// Might as well handle middle click before children as well.
		if (e.button == GLFW_MOUSE_BUTTON_MIDDLE) {
			e.consume(this);
			return;
		}
	}

	Widget::onButton(e);
}


void ScrollWidget::onDragStart(const DragStartEvent& e) {
	e.consume(this);
}


void ScrollWidget::onDragMove(const DragMoveEvent& e) {
	math::Vec offsetDelta = e.mouseDelta.div(getAbsoluteZoom());
	offset = offset.minus(offsetDelta);
}


void ScrollWidget::onHoverScroll(const HoverScrollEvent& e) {
	OpaqueWidget::onHoverScroll(e);
	if (e.isConsumed())
		return;

	// Check if scrollable
	math::Rect offsetBound = getContainerOffsetBound();
	if (offsetBound.getWidth() <= 0.f && offsetBound.getHeight() <= 0.f)
		return;

	math::Vec scrollDelta = e.scrollDelta;
	// Flip coordinates if shift is held
	// Mac (or GLFW?) already does this for us.
#if !defined ARCH_MAC
	int mods = getWindow()->getMods();
	if ((mods & RACK_MOD_MASK) & GLFW_MOD_SHIFT)
		scrollDelta = scrollDelta.flip();
#endif

	offset = offset.minus(scrollDelta);
	e.consume(this);
	internal_->scrolling = true;
}


void ScrollWidget::onHoverKey(const HoverKeyEvent& e) {
	OpaqueWidget::onHoverKey(e);
	if (e.isConsumed())
		return;

	// Check if scrollable
	math::Rect offsetBound = getContainerOffsetBound();
	if (offsetBound.getWidth() <= 0.f && offsetBound.getHeight() <= 0.f)
		return;

	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		if (e.isKeyCommand(GLFW_KEY_PAGE_UP)) {
			offset.setY(offset.getY() - getHeight() * 0.5);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_PAGE_UP, GLFW_MOD_SHIFT)) {
			offset.setX(offset.getX() - getWidth() * 0.5);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_PAGE_DOWN)) {
			offset.setY(offset.getY() + getHeight() * 0.5);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_PAGE_DOWN, GLFW_MOD_SHIFT)) {
			offset.setX(offset.getX() + getWidth() * 0.5);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_HOME)) {
			math::Rect containerBox = container->getVisibleChildrenBoundingBox();
			offset.setY(containerBox.getTop());
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_HOME, GLFW_MOD_SHIFT)) {
			math::Rect containerBox = container->getVisibleChildrenBoundingBox();
			offset.setX(containerBox.getLeft());
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_END)) {
			math::Rect containerBox = container->getVisibleChildrenBoundingBox();
			offset.setY(containerBox.getBottom());
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_END, GLFW_MOD_SHIFT)) {
			math::Rect containerBox = container->getVisibleChildrenBoundingBox();
			offset.setX(containerBox.getRight());
			e.consume(this);
		}
	}
}


} // namespace ui
} // namespace rack
