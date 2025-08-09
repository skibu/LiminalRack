#include <settings.hpp>
#include <ui/MenuEntry.hpp>

namespace rack {
namespace ui {


MenuEntry::MenuEntry() {
    box.size = math::Vec(0, rack::settings::bndWidgetHeight);
}


} // namespace ui
} // namespace rack
