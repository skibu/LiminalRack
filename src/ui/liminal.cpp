#include <ui/liminal.hpp>
#include <settings.hpp>
#include <context.hpp>

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
    // with a whole windowing system. But only do this for Wayland since
    // on other machines like Macs most likely full screen mode would
    // interfere with other applications on the monitor, include IDE when
    // debugging.
    if (isWayland()) {
        rack::settings::windowMaximized = true;
    }

    // So that menus are bigger and easier tp use with touch screen
    rack::settings::bndLabelFontSize = 34; // Increase font size for touch screen
    rack::settings::bndWidgetHeight = rack::settings::bndLabelFontSize + 6; // Increase widget height for touch screen
}


}  // namespace ui
}  // namespace rack