#pragma once
#include <ui/common.hpp>
#include <ui/MenuEntry.hpp>


namespace rack {
namespace ui {

/** A separator line in a menu */
struct MenuSeparator : MenuEntry {
	MenuSeparator();
	void draw(const DrawArgs& args) override;
};


} // namespace ui
} // namespace rack
