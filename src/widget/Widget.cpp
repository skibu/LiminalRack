#include <algorithm>
#include <cxxabi.h> // For demangling on GCC/Clang

#include <widget/Widget.hpp>
#include <context.hpp>


namespace rack {
namespace widget {

// Global for keeping track of widget names, but without changing the Widget
// class itself (to avoid breaking ABI). 
static std::map<Widget*, std::string> widgetNames_g;

Widget::Widget(const std::string& name) {
    // Store the name of the widget
    widgetNames_g[this] = name;
}

Widget::Widget() : Widget("") {}

Widget::~Widget() {
	// You should only delete orphaned widgets
	assert(!parent_);
	clearChildren();

    // Remove from widget names map
    widgetNames_g.erase(this);
}

std::string Widget::getName() {
    auto it = widgetNames_g.find(this);
    if (it != widgetNames_g.end() && !it->second.empty()) {
        // Name was set so return it
        return it->second;
    } else {
		// Name was not set so return class name
		const char* className = typeid(*this).name();

		// Demangle name if using GCC/Clang
#ifdef __GNUC__
		int status;
		char* demangledName =
			abi::__cxa_demangle(className, nullptr, nullptr, &status);
		if (status == 0) {
			className = demangledName;
		}
#endif

		// If have namespace qualifiers, strip them
		std::string classNameStr(className);
		size_t pos = classNameStr.rfind("::");
		if (pos != std::string::npos) {
			return classNameStr.substr(pos + 2);
		}

		return classNameStr;
    }
}

math::Rect Widget::getBox() {
    return box_;
}

void Widget::setBox(math::Rect box) {
	setPos(box.getPos());
	setSize(box.getSize());
}

void Widget::setPos(const math::Vec& pos) {
	if (pos.equals(getPos()))
		return;
	box_.setPos(pos);
	// Dispatch Reposition event
	RepositionEvent eReposition;
	onReposition(eReposition);
}


void Widget::setSize(math::Vec size) {
	if (size.equals(box_.getSize()))
		return;
	box_.setSize(size);

	// Dispatch Resize event
	ResizeEvent eResize;
	onResize(eResize);
}

widget::Widget* Widget::getParent() {
    return parent_;
}

bool Widget::isVisible() {
    return visible_;
}

void Widget::setVisible(bool visible) {
	if (visible == this->visible_)
		return;
	this->visible_ = visible;
	if (visible) {
		// Dispatch Show event
		ShowEvent eShow;
		onShow(eShow);
	}
	else {
		// Dispatch Hide event
		HideEvent eHide;
		onHide(eHide);
	}
}


void Widget::requestDelete() {
    TRACE("requestDelete() called so requestedDelete_ set for widget %s",
          getName().c_str());
    requestedDelete_ = true;
}


math::Rect Widget::getChildrenBoundingBox() {
	math::Vec min = math::Vec(INFINITY, INFINITY);
	math::Vec max = math::Vec(-INFINITY, -INFINITY);
	for (Widget* child : children_) {
		min = min.min(child->box_.getTopLeft());
		max = max.max(child->box_.getBottomRight());
	}
	return math::Rect::fromMinMax(min, max);
}


math::Rect Widget::getVisibleChildrenBoundingBox() {
	math::Vec min = math::Vec(INFINITY, INFINITY);
	math::Vec max = math::Vec(-INFINITY, -INFINITY);
	for (Widget* child : children_) {
		if (!child->isVisible())
			continue;
		min = min.min(child->box_.getTopLeft());
		max = max.max(child->box_.getBottomRight());
	}
	return math::Rect::fromMinMax(min, max);
}


bool Widget::isDescendantOf(Widget* ancestor) {
	if (!parent_)
		return false;
	if (parent_ == ancestor)
		return true;
	return parent_->isDescendantOf(ancestor);
}

math::Vec Widget::getRelativeOffset(const math::Vec v,
                                    Widget* ancestor) {
    // If reach the ancestor, return accumulated offset
    if (this == ancestor) return v;

    // Translate offset
    math::Vec offsetV = v.plus(getPos());

    // If reached the very top, return accumulated offset
    if (parent_ == nullptr) return offsetV;

    // Haven't reached intended ancestor so continue up the parent chain
    return parent_->getRelativeOffset(offsetV, ancestor);
}

float Widget::getRelativeZoom(Widget* ancestor) {
	if (this == ancestor)
		return 1.f;
	if (!parent_)
		return 1.f;
	return parent_->getRelativeZoom(ancestor);
}

math::Vec Widget::getScenePosInLocalCoordsBase(const math::Vec& vec) const {
    if (parent_ != nullptr) {
        // There is a parent so continue to go up the widget hierarchy first.
        // This will cause the offsets to be accumulated back down in the next
        // statements.
        math::Vec localVec = parent_->getScenePosInLocalCoords(vec);

        // Accumulate offset by subtracting parent's position
        return localVec.minus(getPos());
    } else {
        // No parent, so this is the top level widget. Return screenVec adjusted
        // by this
        return vec.minus(getPos());
    }
}

math::Vec Widget::getScenePosInLocalCoords(const math::Vec& vec) const {
	// Handle determining position without zooming
	math::Vec localVec = Widget::getScenePosInLocalCoordsBase(vec);

	// Handle ZoomWidget case. Of course would like to use a virtual function
	// but cannot because plugins compiled to the Rack SDK that inherit from
	// Widget would then have corrupted vtable.
	if (typeid(*this) == typeid(ZoomWidget)) {
		// This is a ZoomWidget so call the special ZoomWidget method
		const ZoomWidget* zoomWidget = static_cast<const ZoomWidget*>(this);
		return zoomWidget->getScreenVecInLocalCoordsForZoomWidget(localVec);
	} else {
		// Normal non-ZoomWidget case, just return localVec
		return localVec;
	}
}

math::Rect Widget::getViewport(math::Rect r) {
	math::Rect bound;
	if (parent_) {
		bound = parent_->getViewport(box_);
	}
	else {
		bound = box_;
	}
	bound.setPos(bound.getPos().minus(box_.getPos()));
	return r.clamp(bound);
}


bool Widget::hasChild(Widget* child) {
	if (!child)
		return false;
	auto it = std::find(children_.begin(), children_.end(), child);
	return (it != children_.end());
}


void Widget::addChild(Widget* child) {
	assert(child);
	assert(!child->parent_);
	// Add child
	child->parent_ = this;
	children_.push_back(child);
	// Dispatch Add event
	AddEvent eAdd;
	child->onAdd(eAdd);
}


void Widget::addChildBottom(Widget* child) {
	assert(child);
	assert(!child->parent_);
	// Add child
	child->parent_ = this;
	children_.push_front(child);
	// Dispatch Add event
	AddEvent eAdd;
	child->onAdd(eAdd);
}


void Widget::addChildBelow(Widget* child, Widget* sibling) {
	assert(child);
	assert(!child->parent_);
	auto it = std::find(children_.begin(), children_.end(), sibling);
	assert(it != children_.end());
	// Add child
	child->parent_ = this;
	children_.insert(it, child);
	// Dispatch Add event
	AddEvent eAdd;
	child->onAdd(eAdd);
}


void Widget::addChildAbove(Widget* child, Widget* sibling) {
	assert(child);
	assert(!child->parent_);
	auto it = std::find(children_.begin(), children_.end(), sibling);
	assert(it != children_.end());
	// Add child
	child->parent_ = this;
	it++;
	children_.insert(it, child);
	// Dispatch Add event
	AddEvent eAdd;
	child->onAdd(eAdd);
}


void Widget::removeChild(Widget* child) {
	assert(child);
	// Make sure `this` is the child's parent
	assert(child->parent_ == this);
	// Dispatch Remove event
	RemoveEvent eRemove;
	child->onRemove(eRemove);
	// Prepare to remove widget from the event state
	getEvent()->finalizeWidget(child);
	// Delete child from children list
	auto it = std::find(children_.begin(), children_.end(), child);
	assert(it != children_.end());
	children_.erase(it);
	// Revoke child's parent
	child->parent_ = nullptr;
}


void Widget::clearChildren() {
	for (Widget* child : children_) {
		// Dispatch Remove event
		RemoveEvent eRemove;
		child->onRemove(eRemove);
		getEvent()->finalizeWidget(child);
		child->parent_ = nullptr;
        delete child;
	}
	children_.clear();
}


void Widget::step() {
	for (auto it = children_.begin(); it != children_.end();) {
		Widget* child = *it;
		// Delete children if a delete is requested
		if (child->requestedDelete_) {
            DEBUG("step(): Starting of deleting child widget %s from parent %s",
                  child->getName().c_str(), getName().c_str());

			// Dispatch Remove event
			RemoveEvent eRemove;
			child->onRemove(eRemove);

            // Update the event state
			getEvent()->finalizeWidget(child);

            // Remove from children list
			it = children_.erase(it);

            // Actually delete the child
            DEBUG("Widget::step(): Deleting child widget %s from parent %s",
                  child->getName().c_str(), getName().c_str());
            child->parent_ = nullptr;
            delete child;

            // Continue to next child
			continue;
		}

        // Not deleting child so step it and then continue on to next child
		child->step();
		it++;
	}
}


void Widget::draw(const DrawArgs& args) {
	// Iterate children
	for (Widget* child : children_) {
		// Don't draw if invisible
		if (!child->isVisible())
			continue;

		// Don't draw if child is outside clip box
		if (!args.clipBox.intersects(child->box_))
			continue;

		drawChild(child, args);
	}
}


void Widget::drawLayer(const DrawArgs& args, int layer) {
	// Iterate children
	for (Widget* child : children_) {
		// Don't draw if invisible
		if (!child->isVisible())
			continue;
            
		// Don't draw if child is outside clip box
		if (!args.clipBox.intersects(child->box_))
			continue;

		drawChild(child, args, layer);
	}
}


void Widget::drawChild(Widget* child, const DrawArgs& args, int layer) {
	DrawArgs childArgs = args;

	// Intersect child clip box with self
	childArgs.clipBox = childArgs.clipBox.intersect(child->box_);

	// Offset clip box by child pos
	childArgs.clipBox.setPos(childArgs.clipBox.getPos().minus(child->box_.getPos()));

	nvgSave(args.vg);
	nvgTranslate(args.vg, child->box_.getX(), child->box_.getY());

	if (layer == 0) {
		child->draw(childArgs);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		// Call deprecated draw function, which does nothing by default
		child->draw(args.vg);
#pragma GCC diagnostic pop
	}
	else {
		child->drawLayer(childArgs, layer);
	}

	nvgRestore(args.vg);
}


} // namespace widget
} // namespace rack
