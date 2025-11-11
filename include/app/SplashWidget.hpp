#pragma once
#include <widget/OpaqueWidget.hpp>

namespace rack {
namespace app {


class SplashWidget : public widget::OpaqueWidget {
   private:
    float fadeAlpha_ = 1.0f;

public:
    SplashWidget();

    /** Called once per frame to update the splash screen. */
    void step() override;

    /** Actually draws the splash screen. */
    void draw(const DrawArgs& args) override;

    /** Keeps calling getWindow()->step() until the splash screen has been
     * viewable long enough. */
    static void waitTillSplashShouldCloseAutomatically(float displayTimeSecs);

    /** Called when the user clicks on the splash screen. Initiates closing of
     * the splash screen. */
    void onButton(const event::Button& e) override;

    /** Starts the fade out process. Though user can click on splash screen to
     * cancel it immediately. */
    static void close();
};

}  // namespace app
}  // namespace rack