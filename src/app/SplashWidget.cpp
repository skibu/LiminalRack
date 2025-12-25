#include <app/SplashWidget.hpp>
#include <context.hpp>
#include <system.hpp>

namespace rack {
namespace app {


SplashWidget::SplashWidget() {}

static bool shouldClose_s = false;
static int stepCount_s = 0;
static const float FADE_OUT_STEPS = 35;

void SplashWidget::step() {
    // Only do fade out if shouldClose_s is true. This way splash screen
    // can stay visible until arbitrary time when close() is called.
    if (shouldClose_s) {
        fadeAlpha_ = (FADE_OUT_STEPS - ++stepCount_s) / FADE_OUT_STEPS;
        if (fadeAlpha_ <= 0.0f) {
            // Done with splash screen so delete it
            requestDelete();
        }
    }

    // Call parent step()
    OpaqueWidget::step();
}

void SplashWidget::draw(const DrawArgs& args) {
    nvgBeginPath(args.vg);

    // Draw darkish background
    nvgRect(args.vg, 0, 0, getParent()->getWidth(), getParent()->getHeight());
    nvgFillColor(args.vg, nvgRGBAf(0.1f, 0.04f, 0.04f, fadeAlpha_));
    nvgFill(args.vg);

    // Load in image using nanovg function, since that is what is used here
    int imageHandle = nvgCreateImage(args.vg, "res/Liminal/liminal-spaces-classroom.png", 
        NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY);
    int imageWidth, imageHeight;
    nvgImageSize(args.vg, imageHandle, &imageWidth, &imageHeight);
    NVGpaint imagePattern = nvgImagePattern(
        args.vg, 0, 0, imageWidth, imageHeight, 0.0f /* angle */, imageHandle, fadeAlpha_);

    // Draw splash image
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 20, 20, imageWidth, imageHeight);
    nvgFillPaint(args.vg, imagePattern);
    nvgFill(args.vg);

    // Logo/Title
    nvgFontSize(args.vg, 60);
    nvgFontFaceId(args.vg, getWindow()->uiFont_->handle);
    nvgFillColor(args.vg, nvgRGBAf(1.0f, 1.0f, 1.0f, fadeAlpha_));
    nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgTextBox(args.vg, getParent()->getWidth() * 0.5f - 350.0f,
               getParent()->getHeight() * 0.4f, 800,
               "Liminal Rack\nAn Instrument or a Computer?", nullptr);

    // Version info
    nvgFontSize(args.vg, 24);
    nvgFillColor(args.vg, nvgRGBAf(0.8f, 0.8f, 0.8f, fadeAlpha_));
    nvgText(args.vg, getParent()->getWidth() * 0.54f,
            getParent()->getHeight() * 0.6f, "Contemplating...", nullptr);
}

void SplashWidget::waitTillSplashShouldCloseAutomatically(
    float displayTimeSecs) {
    // Wait until splash screen should close automatically
    while (!shouldClose_s && system::getTime() < displayTimeSecs) {
        system::sleep(0.1);
        getWindow()->step();
    }

    // Automatically close splash screen since sufficient time has passed
    close();
}

void SplashWidget::onButton(const event::Button& e) {
    if (e.action == GLFW_PRESS) {
        shouldClose_s = true;
    }
    e.consume(this);
}

void SplashWidget::close() {
    shouldClose_s = true;
}

}  // namespace app
}  // namespace rack