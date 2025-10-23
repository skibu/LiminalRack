#include <app/SvgScrew.hpp>
#include <settings.hpp>


namespace rack {
namespace app {


SvgScrew::SvgScrew() {
	fb = new widget::FramebufferWidget;
	addChild(fb);

	sw = new widget::SvgWidget;
	fb->addChild(sw);
}


void SvgScrew::setSvg(std::shared_ptr<window::Svg> svg) {
	if (sw->svg == svg)
		return;

	sw->setSvg(svg);
	fb->setSize(sw->getSize());
	setSize(sw->getSize());

	fb->setDirty();
}


} // namespace app
} // namespace rack
