#include <settings.hpp>
#include <ui/MenuEntry.hpp>

namespace rack {
namespace ui {


MenuEntry::MenuEntry(const std::string& name) : OpaqueWidget(name) {
    setSize(0, rack::settings::bndWidgetHeight);
}

MenuEntry::MenuEntry() : MenuEntry("") {}


} // namespace ui
} // namespace rack
