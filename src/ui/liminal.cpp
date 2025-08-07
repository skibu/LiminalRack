#include <ui/liminal.hpp>
#include <rack.hpp>
#include <settings.hpp>

namespace rack {
namespace ui {

void Liminal::configAsLiminal() {
    rack::settings::isLiminal = true;

    // When in liminal mode then set other params as appropriate
    rack::settings::hasTouchscreen = true;
    rack::settings::hasKeyboard = false; 
    rack::settings::windowMaximized = true;
    rack::settings::bndLabelFontSize = 24; // Increase font size for touch screen
}

}  // namespace ui
}  // namespace rack