/** For "Liminal" mode, where using a touch screen and a Raspberry Pi to
 * simulate a modular synth.
 */

#pragma once

#include "settings.hpp"

namespace rack {
namespace ui {

class Liminal {
   public:
    /** Configures settings for Liminal mode. To be called at startup */
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