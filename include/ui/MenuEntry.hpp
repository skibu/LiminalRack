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
    /** Constructor. Stores name of widget. Can't just use a default value for
     * name param since the original published SDK has a constructor with no
     * parameters. Therefore we provide both constructors.
     */
    MenuEntry(const std::string& name);

    /** Default constructor. Uses empty string for name.
     * Has to be defined in cpp file so that will actually be
     * created and will be accessible to plugins compiled to the legacy Rack
     * SDK.
     */
    MenuEntry();
};

} // namespace ui
} // namespace rack
