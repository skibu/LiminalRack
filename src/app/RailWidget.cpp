#include <app/RailWidget.hpp>
#include <context.hpp>
#include <asset.hpp>
#include <widget/SvgWidget.hpp>
#include <widget/FramebufferWidget.hpp>
#include <settings.hpp>


namespace rack {
namespace app {


struct RailWidget::Internal {
	widget::FramebufferWidget* railFb;
	widget::SvgWidget* railSw;
};


RailWidget::RailWidget() {
    DEBUG("Constructing RailWidget...");

	internal_ = new Internal;

	internal_->railFb = new widget::FramebufferWidget;
	// The rail renders fine without oversampling, and it would be too expensive anyway.
	internal_->railFb->setOversample(1.0);
	// Don't redraw when the world offset of the rail FramebufferWidget changes its fractional value.
	internal_->railFb->setDirtyOnSubpixelChange(false);
	addChild(internal_->railFb);

	internal_->railSw = new widget::SvgWidget;
	internal_->railFb->addChild(internal_->railSw);
}


RailWidget::~RailWidget() {
	delete internal_;
}


void RailWidget::step() {
	// Set rail SVG from theme
	std::shared_ptr<window::Svg> railSvg;
	if (settings::uiTheme == "light") {
		railSvg = window::Svg::load(asset::system("res/ComponentLibrary/Rail-light.svg"));
	}
	else if (settings::uiTheme == "hcdark") {
		railSvg = window::Svg::load(asset::system("res/ComponentLibrary/Rail-hcdark.svg"));
	}
	else {
		// Dark
		railSvg = window::Svg::load(asset::system("res/ComponentLibrary/Rail.svg"));
	}

	if (internal_->railSw->svg != railSvg) {
		internal_->railSw->setSvg(railSvg);
		internal_->railFb->setDirty();
	}

	TransparentWidget::step();
}


void RailWidget::draw(const DrawArgs& args) {
	if (!internal_->railSw->svg)
		return;

	math::Vec tileSize = internal_->railSw->svg->getSize().div(RACK_GRID_SIZE).round().mult(RACK_GRID_SIZE);
	if (tileSize.area() == 0.f)
		return;

	math::Vec min = args.clipBox.getTopLeft().div(tileSize).floor().mult(tileSize);
	math::Vec max = args.clipBox.getBottomRight().div(tileSize).ceil().mult(tileSize);

	// Draw the same FramebufferWidget repeatedly as a tile
	math::Vec p;
	for (p.setY(min.getY()); p.getY() < max.getY(); p.setY(p.getY() + tileSize.getY())) {
		for (p.setX(min.getX()); p.getX() < max.getX(); p.setX(p.getX() + tileSize.getX())) {
			internal_->railFb->setPos(p);
			Widget::drawChild(internal_->railFb, args);
		}
	}
}


} // namespace app
} // namespace rack
