/** For "Liminal" mode, where using a touch screen and a Raspberry Pi to
 * simulate a modular synth.
 */

#pragma once

#include "settings.hpp"

namespace rack {
namespace ui {

class Liminal {
   public:
    /** Configures settings for Liminal mode. To be called at startup.
     * Overrides settings such as isLiminal, hasTouchscreen, and hasKeyboard.
     * Also sets windowMaximized to true for Wayland since on other machines
     * like Macs most likely full screen mode would interfere with other
     * applications on the monitor, include IDE when debugging. Also increases
     * font size.
     * 
     * Overrides the default values from the settings.cpp file. But then
     * these values can be overridden once again when the settings.json file
     * is read in.
     */
    static void configAsLiminal();

    static bool isLiminal() {
        return rack::settings::isLiminal;
    }

    static bool hasTouchscreen() {
        return rack::settings::hasTouchscreen;
    }

    static bool hasKeyboard() {
        return rack::settings::hasKeyboard;
    }
};

}  // namespace ui
}  // namespace rack