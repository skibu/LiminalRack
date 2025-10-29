#include <ui/liminal.hpp>
#include <settings.hpp>

namespace rack {
namespace ui {

void Liminal::configAsLiminal() {
    INFO("Configuring as Liminal Rack mode");

    rack::settings::isLiminal = true;

    // This is a fork of VCV Rack, so some things need to be done differently
    rack::settings::isNotVCVRack = true;

    // When in liminal mode then set other params as appropriate
    rack::settings::hasTouchscreen = true;
    rack::settings::hasKeyboard = false; 

    // Maximize screen since want Liminal to not seem like a computer
    // with a whole windowing system.
    // FIXME commented out setting windowMaximized to true so that can bring
    // up Liminal Rack without full screen. Important since when debugging
    // full screen mode makes it hard to switch to other applications, including debugger.
    // rack::settings::windowMaximized = true;

    // So that menus are bigger and easier tp use with touch screen
    rack::settings::bndLabelFontSize = 24; // Increase font size for touch screen
    rack::settings::bndWidgetHeight = rack::settings::bndLabelFontSize + 8; // Increase widget height for touch screen
}

}  // namespace ui
}  // namespace rack