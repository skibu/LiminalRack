#include <app/SvgKnob.hpp>


namespace rack {
namespace app {


SvgKnob::SvgKnob() {
	fb = new widget::FramebufferWidget;
	addChild(fb);

	shadow = new CircularShadow;
	fb->addChild(shadow);
	shadow->setSize(math::Vec());

	tw = new widget::TransformWidget;
	fb->addChild(tw);

	sw = new widget::SvgWidget;
	tw->addChild(sw);
}

void SvgKnob::setSvg(std::shared_ptr<window::Svg> svg) {
	if (svg == sw->svg)
		return;

	sw->setSvg(svg);
	tw->setSize(sw->getSize());
	fb->setSize(sw->getSize());
	setSize(sw->getSize());

	shadow->setSize(sw->getSize());

	// Move shadow downward by 10%
	shadow->setPos(math::Vec(0, sw->getHeight() * 0.10));

	fb->setDirty();
}

void SvgKnob::onChange(const ChangeEvent& e) {
	float angle = 0.f;

	// Calculate angle from value
	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		float value = pq->getValue();
		if (!pq->isBounded()) {
			// Number of rotations equals value for unbounded range
			angle = value * (2 * M_PI);
		}
		else if (pq->getRange() == 0.f) {
			// Center angle for zero range
			angle = (minAngle + maxAngle) / 2.f;
		}
		else {
			// Proportional angle for finite range
			angle = math::rescale(value, pq->getMinValue(), pq->getMaxValue(), minAngle, maxAngle);
		}
		angle = std::fmod(angle, 2 * M_PI);
	}

	tw->identity();
	// Rotate SVG
	math::Vec center = sw->getBox().getCenter();
	tw->translate(center);
	tw->rotate(angle);
	tw->translate(center.neg());
	fb->setDirty();

	Knob::onChange(e);
}


} // namespace app
} // namespace rack
