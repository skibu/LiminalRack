#include <app/SplashWidget.hpp>
#include <context.hpp>
#include <system.hpp>

namespace rack {
namespace app {


SplashWidget::SplashWidget() {}

static bool shouldClose_s = false;
static int stepCount_s = 0;
static const float FADE_OUT_STEPS = 35;
static int image_handle_s = -1;

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

/** Loads a random image from the "res/splashScreen" directory. Can handle
 * image formats supported by NanoVG which includes PNG and JPEG. Caches the
 * image handle so that the same image is used on subsequent calls.
 */
static int getRandomImageHandle(NVGcontext* vg) {
  if (image_handle_s < 0) {
    std::string images_dir = "res/splashScreen";
    std::vector<std::string> images_file_names = system::getEntries(images_dir);

    int index = std::rand() % images_file_names.size();
    std::string random_image_path = images_file_names[index];

    // For now, just return a fixed image handle
    image_handle_s = nvgCreateImage(vg, random_image_path.c_str(),
                                    0 /* FIXME NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY) */);
  }

  return image_handle_s;
}

void SplashWidget::draw(const DrawArgs& args) {
    // Determine image scaling so that 
    float windowWidth = getParent()->getWidth();
    float windowHeight = getParent()->getHeight();

    nvgBeginPath(args.vg);

    // Draw darkish background
    nvgRect(args.vg, 0, 0, windowWidth, windowHeight);
    NVGcolor bgColor = nvgRGBAf(0.1f, 0.04f, 0.04f, fadeAlpha_);
    nvgFillColor(args.vg, bgColor);
    nvgFill(args.vg);

    // Get a random image from the "res/splashScreen" directory
    int imageHandle = getRandomImageHandle(args.vg);

    // Determine unscaled image size
    int imageWidth, imageHeight;
    nvgImageSize(args.vg, imageHandle, &imageWidth, &imageHeight);

    // Determine image scaling so that image fits within window with some margin
    const float imageMargin = 10.0f;
    float imageScaling =
        std::min((windowWidth * 0.75f) / imageWidth,
                 (windowHeight - 2 * imageMargin) / imageHeight);

    // Create image pattern
    float imageYMargin = (windowHeight - imageScaling * imageHeight) / 2;
    NVGpaint imagePattern = nvgImagePattern(
        args.vg, imageMargin, imageYMargin, imageWidth * imageScaling,
        imageHeight * imageScaling, 0.0f /* angle */, imageHandle, fadeAlpha_);

    // Draw splash image
    nvgBeginPath(args.vg);
    nvgRect(args.vg, imageMargin, imageYMargin, imageWidth * imageScaling,
            imageHeight * imageScaling);
    nvgFillPaint(args.vg, imagePattern);
    nvgFill(args.vg);

    // Display text
    nvgFontFaceId(args.vg, getWindow()->uiFont_->handle);
    const NVGcolor fontColor = nvgRGBAf(1.0f, 1.0f, 1.0f, fadeAlpha_);

    // Parameters for drawing text
    const float title1FontSize = 68.f;
    const float title2FontSize = 46.f;
    const float additionalTextFontSize = 24.f;
    const float rightMargin = 20.f;
    const float textBgPadding = 20.f;
    const float textBgOpacity = 0.40f;
    NVGcolor textBgColor = color::alpha(bgColor, textBgOpacity); 

    // Figure out width of text1
    std::string title1 = string::translate("splashScreen.title1");
    nvgFontSize(args.vg, title1FontSize);
    float bounds1[4];  // xMin, yMin, xMax, yMax
    nvgTextBounds(args.vg, 0, 0, title1.c_str(), nullptr, bounds1);
    float textWidth1 = bounds1[2] - bounds1[0];
    float maxTextWidth = textWidth1;
    float textCenterX = windowWidth - maxTextWidth/ 2 - rightMargin;
    float textHeight1 = bounds1[3] - bounds1[1];
    float textY1 = windowHeight * 0.3f;
    float nextLineY = textY1 + textHeight1;

    // Draw text2 if it is set
    std::string title2 = string::translate("splashScreen.title2");
    if (title2 != "") {
        // If text2 is wider, use that width for centering all text
        nvgFontSize(args.vg, title2FontSize);
        float bounds2[4];  // xMin, yMin, xMax, yMax
        nvgTextBounds(args.vg, 0, 0, title2.c_str(), nullptr, bounds2);
        float textWidth2 = bounds2[2] - bounds2[0];
        float textHeight2 = bounds2[3] - bounds2[1];

        if (textWidth2 > maxTextWidth) {
            maxTextWidth = textWidth2;
            textCenterX = windowWidth - maxTextWidth / 2 - rightMargin;
        }

        nextLineY += 0;  // Extra space between lines, if needed. Currently none.

        // Draw background box for text2 so it is readable over image
        nvgFillColor(args.vg, textBgColor);
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, textCenterX - textWidth2 / 2 - textBgPadding,
                       nextLineY - title2FontSize + bounds2[3],
                       textWidth2 + 2 * textBgPadding, textHeight2,
                       20.f /* radius */);
        nvgFill(args.vg);

        // Draw text2
        nvgFillColor(args.vg, fontColor);
        nvgText(args.vg, textCenterX - textWidth2 / 2, nextLineY,
                title2.c_str(), nullptr);

        // For subsequent text, move down
        nextLineY += textHeight2;
    }   

    // Draw background box for text1 so it is readable over image
    nvgFillColor(args.vg, textBgColor);
    nvgBeginPath(args.vg);
    nvgRoundedRect(args.vg, textCenterX - textWidth1 / 2 - textBgPadding,
                    textY1 - title1FontSize + bounds1[3],
                    textWidth1 + 2 * textBgPadding, textHeight1,
                    20.f /* radius */);
    nvgFill(args.vg);

    // Draw text1
    nvgFontSize(args.vg, title1FontSize);
    nvgFillColor(args.vg, fontColor);
    nvgText(args.vg, textCenterX - textWidth1 / 2, textY1, title1.c_str(),
            nullptr);

    // Draw background box for additional text so it is readable over image
    nvgFontSize(args.vg, additionalTextFontSize);
    std::string additionalText =
        string::translate("splashScreen.additionalText");
    float boundsAdditional[4];  // xMin, yMin, xMax, yMax
    nvgTextBounds(args.vg, 0, 0, additionalText.c_str(), nullptr, boundsAdditional);
    float additionalTextWidth = boundsAdditional[2] - boundsAdditional[0];

    nvgFillColor(args.vg, textBgColor);
    nvgBeginPath(args.vg);
    nvgRoundedRect(
        args.vg, textCenterX - additionalTextWidth / 2 - textBgPadding,
        nextLineY + 20 - additionalTextFontSize + boundsAdditional[3],
        additionalTextWidth + 2 * textBgPadding, additionalTextFontSize,
        20.f /* radius */);
    nvgFill(args.vg);

    // Draw additional text
    nvgFillColor(args.vg, fontColor);
    nvgText(args.vg, textCenterX - additionalTextWidth / 2, nextLineY + 20,
            additionalText.c_str(), nullptr);
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