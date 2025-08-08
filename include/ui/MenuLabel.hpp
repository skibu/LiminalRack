#pragma once
#include <ui/common.hpp>
#include <ui/MenuEntry.hpp>


namespace rack {
namespace ui {

/** A non-selectable label that can be an item in a menu */
struct MenuLabel : MenuEntry {
	std::string text;

	void draw(const DrawArgs& args) override;
	void step() override;
};


} // namespace ui
} // namespace rack
