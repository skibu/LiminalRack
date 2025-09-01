#pragma once
#include <widget/Widget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {


struct Tooltip : widget::Widget {
    /** Text to display in the tooltip */
	std::string text;

    /** Position and size the tooltip */
	void step() override;

    /** Actually draw the tooltip */
	void draw(const DrawArgs& args) override;
};


} // namespace ui
} // namespace rack
