#include <app/SvgPort.hpp>


namespace rack {
namespace app {


SvgPort::SvgPort() {
	fb = new widget::FramebufferWidget;
	addChild(fb);

	shadow = new CircularShadow;
	fb->addChild(shadow);
	// Avoid breakage if plugins fail to call setSvg()
	// In that case, just disable the shadow.
	shadow->setSize(math::Vec());

	sw = new widget::SvgWidget;
	fb->addChild(sw);
}

void SvgPort::setSvg(std::shared_ptr<window::Svg> svg) {
	if (svg == sw->svg)
		return;

	sw->setSvg(svg);
	fb->setSize(sw->getSize());
	setSize(sw->getSize());

	// Move shadow downward by 10%
	shadow->setSize(sw->getSize());
	shadow->setPos(math::Vec(0, sw->getHeight() * 0.10));

	fb->setDirty();
}


} // namespace app
} // namespace rack
