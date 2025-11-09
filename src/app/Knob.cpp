#include <app/Knob.hpp>
#include <context.hpp>
#include <app/Scene.hpp>
#include <app/RackScrollWidget.hpp>
#include <random.hpp>
#include <history.hpp>
#include <settings.hpp>


namespace rack {
namespace app {


struct Knob::Internal {
	/** Value of the knob before dragging. */
	float oldValue = NAN;

	/** Fractional value between the param's value and the dragged knob position.
	Using a "snapValue" variable and rounding is insufficient because the mouse needs to reach 1.0, not 0.5 to obtain the first increment.
	*/
	float snapDelta = 0.f;

	/** Speed multiplier in speed knob mode */
	float linearScale = 1.f;
    
	/** The mouse has once escaped from the knob while dragging. */
	bool rotaryDragEnabled = false;
	float dragAngle = NAN;

	float distDragged = 0.f;

    /** Whether the knob is currently being dragged. */
    bool dragging_ = false;
};


Knob::Knob() {
	internal_ = new Internal;
}

Knob::~Knob() {
	delete internal_;
}

void Knob::initParamQuantity() {
	ParamWidget::initParamQuantity();
	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		if (snap)
			pq->snapEnabled = true;
		// Only enable smoothing if snapping is disabled
		if (smooth && !pq->snapEnabled)
			pq->smoothEnabled = true;
	}
}

void Knob::onHover(const HoverEvent& e) {
	// Only call super if mouse position is in the circle
	math::Vec c = getSize().div(2);
    float dist = e.pos.minus(c).norm();
	if (dist <= c.getX()) {
		ParamWidget::onHover(e);
	}
}

void Knob::onButton(const ButtonEvent& e) {
	math::Vec c = getSize().div(2);
	float dist = e.pos.minus(c).norm();
	if (dist <= c.getX()) {
		ParamWidget::onButton(e);
	}
}

void Knob::onDragStart(const DragStartEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

    // Mark as dragging
    internal_->dragging_ = true;

	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		internal_->oldValue = pq->getValue();
		internal_->snapDelta = 0.f;
	}

	settings::KnobMode km = settings::knobMode;
	if (km == settings::KNOB_MODE_LINEAR || km == settings::KNOB_MODE_SCALED_LINEAR) {
		getWindow()->cursorLock();
	}
	// Only changed for KNOB_MODE_LINEAR_*.
	internal_->linearScale = 1.f;
	// Only used for KNOB_MODE_ROTARY_*.
	internal_->rotaryDragEnabled = false;
	internal_->dragAngle = NAN;

	// Reset distance dragged
	internal_->distDragged = 0.f;

	ParamWidget::onDragStart(e);
}

void Knob::onDragEnd(const DragEndEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

    // Mark as done dragging
    internal_->dragging_ = false;

	settings::KnobMode km = settings::knobMode;
	if (km == settings::KNOB_MODE_LINEAR || km == settings::KNOB_MODE_SCALED_LINEAR) {
		getWindow()->cursorUnlock();
	}

	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		float newValue = pq->getValue();
		if (!std::isnan(internal_->oldValue) && internal_->oldValue != newValue) {
			// Push ParamChange history action
			history::ParamChange* h = new history::ParamChange;
			h->name = string::translate("Knob.history.move");
			h->moduleId = module->id;
			h->paramId = paramId;
			h->oldValue = internal_->oldValue;
			h->newValue = newValue;
			getHistory()->push(h);
		}
		// Reset snap delta
		internal_->snapDelta = 0.f;
	}
	internal_->oldValue = NAN;

	// Dispatch Action event if mouse traveled less than a threshold distance
	const float actionDistThreshold = 16.f;
	if (internal_->distDragged < actionDistThreshold) {
		ActionEvent eAction;
		onAction(eAction);
	}

	ParamWidget::onDragEnd(e);
}

static float getModSpeed() {
	int mods = getWindow()->getMods();
	if ((mods & RACK_MOD_MASK) == RACK_MOD_CTRL)
		return 1 / 10.f;
	else if ((mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT)
		return 4.f;
	else if ((mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT))
		return 1 / 100.f;
	else
		return 1.f;
}

void Knob::onDragMove(const DragMoveEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	settings::KnobMode km = settings::knobMode;
	bool linearMode = (km == settings::KNOB_MODE_LINEAR || km == settings::KNOB_MODE_SCALED_LINEAR) || forceLinear;

	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		float value = pq->getValue();

		// Ratio between parameter value scale / (angle range / 2*pi)
		float rangeRatio;
		if (pq->isBounded()) {
			rangeRatio = pq->getRange();
			rangeRatio /= (maxAngle - minAngle) / float(2 * M_PI);
		}
		else {
			rangeRatio = 1.f;
		}

		if (linearMode) {
			float delta = (horizontal ? e.mouseDelta.getX() : -e.mouseDelta.getY());
			delta *= settings::knobLinearSensitivity;
			delta *= speed;
			delta *= getModSpeed();
			delta *= rangeRatio;

			// Scale delta if in scaled linear knob mode
			if (km == settings::KNOB_MODE_SCALED_LINEAR) {
				float deltaY = (horizontal ? -e.mouseDelta.getY() : -e.mouseDelta.getX());
				const float pixelTau = 200.f;
				internal_->linearScale *= std::pow(2.f, -deltaY / pixelTau);
				delta *= internal_->linearScale;
			}

			// Handle value snapping
			if (pq->snapEnabled) {
				// Replace delta with an accumulated delta since the last integer knob.
				internal_->snapDelta += delta;
				delta = std::trunc(internal_->snapDelta);
				internal_->snapDelta -= delta;
			}

			value += delta;
		}
		else if (internal_->rotaryDragEnabled) {
			math::Vec origin = getInSceneCoords(getSize().div(2));
			math::Vec deltaPos = getScene()->getMousePos().minus(origin);
			float angle = deltaPos.arg() + float(M_PI) / 2;

			bool absoluteRotaryMode = (km == settings::KNOB_MODE_ROTARY_ABSOLUTE) && pq->isBounded();
			if (absoluteRotaryMode) {
				// Find angle closest to midpoint of angle range, mod 2*pi
				float midAngle = (minAngle + maxAngle) / 2;
				angle = math::eucMod(angle - midAngle + float(M_PI), float(2 * M_PI)) + midAngle - float(M_PI);
				value = math::rescale(angle, minAngle, maxAngle, pq->getMinValue(), pq->getMaxValue());
			}
			else {
				if (!std::isfinite(internal_->dragAngle)) {
					// Set the starting angle
					internal_->dragAngle = angle;
				}

				// Find angle closest to last angle, mod 2*pi
				float deltaAngle = math::eucMod(angle - internal_->dragAngle + float(M_PI), float(2 * M_PI)) - float(M_PI);
				internal_->dragAngle = angle;
				float delta = deltaAngle / float(2 * M_PI) * rangeRatio;
				delta *= getModSpeed();

				// Handle value snapping
				if (pq->snapEnabled) {
					// Replace delta with an accumulated delta since the last integer knob.
					internal_->snapDelta += delta;
					delta = std::trunc(internal_->snapDelta);
					internal_->snapDelta -= delta;
				}

				value += delta;
			}
		}

		// Set value
		pq->setValue(value);
	}

	internal_->distDragged += e.mouseDelta.norm();

	ParamWidget::onDragMove(e);
}

void Knob::onDragLeave(const DragLeaveEvent& e) {
	if (e.origin == this) {
		internal_->rotaryDragEnabled = true;
	}

	ParamWidget::onDragLeave(e);
}


void Knob::onHoverScroll(const HoverScrollEvent& e) {
	ParamWidget::onHoverScroll(e);

	if (!settings::knobScroll)
		return;

	if (getScene()->getRackScroll()->isScrolling())
		return;

	engine::ParamQuantity* pq = getParamQuantity();
	if (!pq)
		return;

	float value = pq->getValue();
	// Set old value if unset
	if (std::isnan(internal_->oldValue)) {
		internal_->oldValue = value;
	}

	float rangeRatio;
	if (pq->isBounded()) {
		rangeRatio = pq->getRange();
	}
	else {
		rangeRatio = 1.f;
	}

	// Calculate delta value
	float delta = e.scrollDelta.getY();
	delta *= settings::knobScrollSensitivity;
	delta *= speed;
	delta *= getModSpeed();
	delta *= rangeRatio;

	// Handle value snapping
	if (pq->snapEnabled) {
		// Replace delta with an accumulated delta since the last integer knob.
		internal_->snapDelta += delta;
		delta = std::trunc(internal_->snapDelta);
		internal_->snapDelta -= delta;
	}

	value += delta;
	pq->setValue(value);

	e.consume(this);
}


void Knob::onLeave(const LeaveEvent& e) {
	ParamWidget::onLeave(e);

	if (!settings::knobScroll)
		return;

	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		float newValue = pq->getValue();
		if (!std::isnan(internal_->oldValue) && internal_->oldValue != newValue) {
			// Push ParamChange history action
			history::ParamChange* h = new history::ParamChange;
			h->name = string::translate("Knob.history.move");
			h->moduleId = module->id;
			h->paramId = paramId;
			h->oldValue = internal_->oldValue;
			h->newValue = newValue;
			getHistory()->push(h);
		}
		// Reset snap delta
		internal_->snapDelta = 0.f;
	}
	internal_->oldValue = NAN;
}

void Knob::draw(const DrawArgs& args) {
    // Call parent draw to actually draw the knob
    ParamWidget::draw(args);

    // If currently manipulating the knob, highlight it slightly
    // to make it clear that it is being turned
    if (internal_->dragging_) {
        // Setup drawing of highlight
        NVGcontext* vg = args.vg;
        nvgBeginPath(vg);

        float heightToWidthRatio = getHeight() / getWidth();
        if (heightToWidthRatio < 0.9f || heightToWidthRatio > 1.1f) {
            // Use a rounded rectangle for non-circular knobs (e.g. sliders)
            float radius = std::min(getWidth(), getHeight()) / 4.0f;
            nvgRoundedRect(vg, 0, -getHeight() * 0.05f, getWidth(),
                           getHeight() * 1.1f, radius);

            float strokeWidth = math::clamp(getWidth() / 3.0f, 3.0f, 10.0f);
            nvgStrokeWidth(vg, strokeWidth);
        } else {
            // Use an circle for circular knobs, where width ~= height
            math::Vec center = getSize().div(2);
            float radius = getWidth() / 2.8f;
            nvgCircle(vg, center.getX(), center.getY(), radius);

            float strokeWidth = math::clamp(getWidth() / 2.7f, 5.0f, 20.0f);
            nvgStrokeWidth(vg, strokeWidth);
        }

        // Use a green stroke that is mostly transparent
        nvgStrokeColor(vg, nvgRGBAf(0.5f, 1.0f, 0.5f, 0.3f));

        // Actually draw the stroke
        nvgStroke(vg);
    }
}

} // namespace app
} // namespace rack
