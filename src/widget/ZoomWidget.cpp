#include <widget/ZoomWidget.hpp>


namespace rack {
namespace widget {


math::Vec ZoomWidget::getRelativeOffset(const math::Vec& v, Widget* ancestor) const {
	// Transform `v` (which is in child coordinates) to local coordinates.
	math::Vec adjustedV = v.mult(zoom_);
	return Widget::getRelativeOffset(adjustedV, ancestor);
}


float ZoomWidget::getRelativeZoom(Widget* ancestor) {
	return zoom_ * Widget::getRelativeZoom(ancestor);
}

math::Vec ZoomWidget::getScenePosInLocalCoords(const math::Vec& v) const {
    // Call the base Widget implementation to get local coords without zoom
    math::Vec localVec = Widget::getScenePosInLocalCoords(v);

    // Adjust for zoom and return value
    math::Vec adjustedV = localVec.div(zoom_);
    return adjustedV;
}

math::Rect ZoomWidget::getViewport(math::Rect r) {
	r.setPos(r.getPos().mult(zoom_));
	r.setSize(r.getSize().mult(zoom_));
	r = Widget::getViewport(r);
	r.setPos(r.getPos().div(zoom_));
	r.setSize(r.getSize().div(zoom_));
	return r;
}


float ZoomWidget::getZoom() {
	return zoom_;
}


void ZoomWidget::setZoom(float zoom) {
	if (zoom == this->zoom_)
		return;
	this->zoom_ = zoom;

	// Dispatch Dirty event
	widget::EventContext cDirty;
	DirtyEvent eDirty;
	eDirty.context = &cDirty;
	Widget::onDirty(eDirty);
}


void ZoomWidget::draw(const DrawArgs& args) {
	DrawArgs zoomCtx = args;
	zoomCtx.clipBox.setPos(zoomCtx.clipBox.getPos().div(zoom_));
	zoomCtx.clipBox.setSize(zoomCtx.clipBox.getSize().div(zoom_));
    
	// No need to save the state because that is done in the parent
	nvgScale(args.vg, zoom_, zoom_);
	Widget::draw(zoomCtx);
}


void ZoomWidget::drawLayer(const DrawArgs& args, int layer) {
	DrawArgs zoomCtx = args;
	zoomCtx.clipBox.setPos(zoomCtx.clipBox.getPos().div(zoom_));
	zoomCtx.clipBox.setSize(zoomCtx.clipBox.getSize().div(zoom_));

	// No need to save the state because that is done in the parent
	nvgScale(args.vg, zoom_, zoom_);
	Widget::drawLayer(zoomCtx, layer);
}


} // namespace widget
} // namespace rack
