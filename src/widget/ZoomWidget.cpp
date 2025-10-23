#include <widget/ZoomWidget.hpp>


namespace rack {
namespace widget {


math::Vec ZoomWidget::getRelativeOffset(math::Vec v, Widget* ancestor) {
	// Transform `v` (which is in child coordinates) to local coordinates.
	v = v.mult(zoom);
	return Widget::getRelativeOffset(v, ancestor);
}


float ZoomWidget::getRelativeZoom(Widget* ancestor) {
	return zoom * Widget::getRelativeZoom(ancestor);
}


math::Rect ZoomWidget::getViewport(math::Rect r) {
	r.setPos(r.getPos().mult(zoom));
	r.setSize(r.getSize().mult(zoom));
	r = Widget::getViewport(r);
	r.setPos(r.getPos().div(zoom));
	r.setSize(r.getSize().div(zoom));
	return r;
}


float ZoomWidget::getZoom() {
	return zoom;
}


void ZoomWidget::setZoom(float zoom) {
	if (zoom == this->zoom)
		return;
	this->zoom = zoom;

	// Dispatch Dirty event
	widget::EventContext cDirty;
	DirtyEvent eDirty;
	eDirty.context = &cDirty;
	Widget::onDirty(eDirty);
}


void ZoomWidget::draw(const DrawArgs& args) {
	DrawArgs zoomCtx = args;
	zoomCtx.clipBox.setPos(zoomCtx.clipBox.getPos().div(zoom));
	zoomCtx.clipBox.setSize(zoomCtx.clipBox.getSize().div(zoom));
    
	// No need to save the state because that is done in the parent
	nvgScale(args.vg, zoom, zoom);
	Widget::draw(zoomCtx);
}


void ZoomWidget::drawLayer(const DrawArgs& args, int layer) {
	DrawArgs zoomCtx = args;
	zoomCtx.clipBox.setPos(zoomCtx.clipBox.getPos().div(zoom));
	zoomCtx.clipBox.setSize(zoomCtx.clipBox.getSize().div(zoom));

	// No need to save the state because that is done in the parent
	nvgScale(args.vg, zoom, zoom);
	Widget::drawLayer(zoomCtx, layer);
}


} // namespace widget
} // namespace rack
