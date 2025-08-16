#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

/**
 * A Menu entry that could be clickable or just a label.
 * Just sets the box size to Vec(0, :settings::bndWidgetHeight)
 */
struct MenuEntry : widget::OpaqueWidget {
	MenuEntry();
};


} // namespace ui
} // namespace rack
