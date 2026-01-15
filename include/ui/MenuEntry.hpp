#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

/**
 * A Menu entry that could be clickable or just a label.
 * Just sets the box size to Vec(0, :settings::bndWidgetHeight)
 */
class MenuEntry : public widget::OpaqueWidget {
   public:
    MenuEntry(const std::string& name = "");
};

} // namespace ui
} // namespace rack
