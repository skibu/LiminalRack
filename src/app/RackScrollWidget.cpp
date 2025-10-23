#include <app/RackScrollWidget.hpp>
#include <app/Scene.hpp>
#include <app/RackWidget.hpp>
#include <app/ModuleWidget.hpp>
#include <app/PortWidget.hpp>
#include <window/Window.hpp>
#include <context.hpp>
#include <settings.hpp>


namespace rack {
namespace app {


struct RackScrollWidget::Internal {
	/** For viewport expanding */
	float oldZoom = 0.f;
	math::Vec oldOffset;
};


RackScrollWidget::RackScrollWidget() {
	internal_ = new Internal;

	zoomWidget = new widget::ZoomWidget;
	container->addChild(zoomWidget);

	rackWidget = new RackWidget;
	rackWidget->setSize(RACK_OFFSET.mult(2));
	zoomWidget->addChild(rackWidget);

	reset();
}


RackScrollWidget::~RackScrollWidget() {
	delete internal_;
}


void RackScrollWidget::reset() {
	offset = RACK_OFFSET * zoomWidget->getZoom() - math::Vec(30, 30);
}


math::Vec RackScrollWidget::getGridOffset() {
	return (offset / zoomWidget->getZoom() - RACK_OFFSET) / RACK_GRID_SIZE;
}


void RackScrollWidget::setGridOffset(math::Vec gridOffset) {
	offset = (gridOffset * RACK_GRID_SIZE + RACK_OFFSET) * zoomWidget->getZoom();
}


float RackScrollWidget::getZoom() {
	return zoomWidget->getZoom();
}


void RackScrollWidget::setZoom(float zoom) {
	setZoom(zoom, getSize().div(2));
}


void RackScrollWidget::setZoom(float zoom, math::Vec pivot) {
	zoom = math::clamp(zoom, std::pow(2.f, -2), std::pow(2.f, 2));

	offset = (offset + pivot) * (zoom / zoomWidget->getZoom()) - pivot;
	zoomWidget->setZoom(zoom);
}


void RackScrollWidget::zoomToModules() {
	// Determine bounding box of the existing modules
	widget::Widget* moduleContainer = rackWidget->getModuleContainer();
	math::Rect bound = moduleContainer->getChildrenBoundingBox();

	// Zoom to the modules
	zoomToBound(bound);
}


void RackScrollWidget::zoomToBound(math::Rect bound) {
	if (!bound.getPos().isFinite())
		return;

	// Originally the boundary was expanded by 24 units, presumably to show extra rails
	// to idicate that more space is available. But for Liminal have a relatively small
	// screen and don't want to waste any space.
	if (!rack::settings::isNotVCVRack) {
		bound = bound.grow(math::Vec(24, 24));
	}
	
	math::Vec size = getSize();
	float zoom = std::min(size.getX() / bound.getWidth(), size.getY() / bound.getHeight());
	offset = bound.getCenter() * zoom - size / 2;
	zoomWidget->setZoom(zoom);
}


void RackScrollWidget::step() {
	float zoom = getZoom();

	// Compute module bounding box
	math::Rect moduleBox = rackWidget->getModuleContainer()->getChildrenBoundingBox();
	if (!moduleBox.getSize().isFinite())
		moduleBox = math::Rect(RACK_OFFSET, math::Vec(0, 0));

	// Expand moduleBox by a screen size
	math::Rect scrollBox = moduleBox;
	scrollBox.setPos(scrollBox.getPos().mult(zoom));
	scrollBox.setSize(scrollBox.getSize().mult(zoom));
	scrollBox = scrollBox.grow(getSize().mult(0.9));

	// Expand to the current viewport box so that moving modules (and thus changing the module bounding box) doesn't clamp the scroll offset.
	if (zoom == internal_->oldZoom) {
		math::Rect viewportBox;
		viewportBox.setPos(internal_->oldOffset);
		viewportBox.setSize(getSize());
		scrollBox = scrollBox.expand(viewportBox);
	}

	// Reposition widgets
	zoomWidget->setBox(scrollBox);
	rackWidget->setPos(scrollBox.getPos().div(zoom).neg());

	// Scroll rack if dragging certain widgets near the edge of the screen
	math::Vec pos = getScene()->getMousePos() - getPos();
	math::Rect viewport = getViewport(getBox().zeroPos());
	widget::Widget* dw = getEvent()->getDraggedWidget();
	if (dw && getEvent()->dragButton == GLFW_MOUSE_BUTTON_LEFT &&
		(dynamic_cast<RackWidget*>(dw) || dynamic_cast<ModuleWidget*>(dw) || dynamic_cast<PortWidget*>(dw))) {
		float margin = 1.0;
		float speed = 15.0;
		if (pos.getX() <= viewport.getPosX() + margin)
			offset = math::Vec(offset.getX() - speed, offset.getY());
		if (pos.getX() >= viewport.getPosX() + viewport.getWidth() - margin)
			offset = math::Vec(offset.getX() + speed, offset.getY());
		if (pos.getY() <= viewport.getPosY() + margin)
			offset = math::Vec(offset.getX(), offset.getY() - speed);
		if (pos.getY() >= viewport.getPosY() + viewport.getHeight() - margin)
			offset = math::Vec(offset.getX(), offset.getY() + speed);
	}

	// Hide scrollbars if fullscreen
	hideScrollbars = getWindow()->isFullScreen();

    //FIXME
    static int count = 0;
    if (count++ % 180 == 0) {
        math::Vec gridOffset = getGridOffset();
        INFO("RackScrollWidget: zoom %f, offset %f,%f, gridOffset %f,%f", zoom, offset.getX(), offset.getY(), gridOffset.getX(), gridOffset.getY());
    }

	ScrollWidget::step();

	internal_->oldOffset = offset;
	internal_->oldZoom = zoom;
}


void RackScrollWidget::draw(const DrawArgs& args) {
	ScrollWidget::draw(args);
}


void RackScrollWidget::onHoverKey(const HoverKeyEvent& e) {
	ScrollWidget::onHoverKey(e);
}


void RackScrollWidget::onHoverScroll(const HoverScrollEvent& e) {
	int mods = getWindow()->getMods();
	bool doZoom = mods & RACK_MOD_CTRL;
	if (settings::mouseWheelZoom)
		doZoom ^= true;

	if (doZoom) {
		// Dispatch to children first and zoom only if they don't consume
		OpaqueWidget::onHoverScroll(e);
		if (e.isConsumed())
			return;
		// Increase zoom
		float zoomDelta = e.scrollDelta.getY() / 50 / 4;
		if (settings::invertZoom)
			zoomDelta *= -1;
		float zoom = getZoom() * std::pow(2.f, zoomDelta);
		setZoom(zoom, e.pos);
		e.consume(this);
		return;
	}

	ScrollWidget::onHoverScroll(e);
}


void RackScrollWidget::onHover(const HoverEvent& e) {
	ScrollWidget::onHover(e);

	// Hide menu bar if fullscreen and moving mouse over the RackScrollWidget
	if (getWindow()->isFullScreen()) {
		getScene()->getMenuBar()->hide();
	}
}


void RackScrollWidget::onButton(const ButtonEvent& e) {
	ScrollWidget::onButton(e);
	if (e.isConsumed())
		return;

	// Zoom in/out with extra mouse buttons
	if (e.action == GLFW_PRESS) {
		if (e.button == GLFW_MOUSE_BUTTON_4) {
			float zoom = getZoom() * std::pow(2.f, -0.5f);
			setZoom(zoom, e.pos);
			e.consume(this);
		}
		if (e.button == GLFW_MOUSE_BUTTON_5) {
			float zoom = getZoom() * std::pow(2.f, 0.5f);
			setZoom(zoom, e.pos);
			e.consume(this);
		}
	}
}


} // namespace app
} // namespace rack
