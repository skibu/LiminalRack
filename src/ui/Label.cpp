#include <ui/Label.hpp>
#include <settings.hpp>
#include <context.hpp>
#include <asset.hpp>

namespace rack {
namespace ui {

/** Constructor with param for initialing text to a value. Does actual
 * initialization using the default constructor. */
Label::Label(const std::string& initialText) : Label() {
    setText(initialText);
}

Label::Label() {
    fontSize_ = rack::settings::bndLabelFontSize;
    setHeight(fontSize_ + 12);
    lineHeight_ = 1.2;
    yOffset_ = 0.0f;
    color_ = color::BLACK_TRANSPARENT;
}

void Label::draw(const DrawArgs& args) {
    // For debugging one can draw rectangle so that can see the rectanble used
    // for the Label
    // bndBackgroundColor(args.vg, 0.0, 0.0, box.size.x, box.size.y, 0,
    // color::RED, color::GREEN);

    // Temporarily set to special font face if so configured
    if (!fontFaceOverride_.empty()) {
      getWindow()->overrideFontFace(asset::system(fontFaceOverride_));
    }
    
    // Align the text as specified.
    float x;
    switch (alignment_) {
        default:
        case LEFT_ALIGNMENT: {
            x = 0.0;
        } break;
        case RIGHT_ALIGNMENT: {
            x = getWidth() -
                bndLabelWidthForFontSize(args.vg, -1, fontSize_, text_.c_str());
        } break;
        case CENTER_ALIGNMENT: {
            x = (getWidth() - bndLabelWidthForFontSize(args.vg, -1, fontSize_,
                                                       text_.c_str())) /
                2.0;
        } break;
    }

    nvgTextLineHeight(args.vg, lineHeight_);
    NVGcolor colorActual =
        (color_.a > 0.f) ? color_ : bndGetTheme()->regularTheme.textColor;
    bndIconLabelValue(args.vg, x, yOffset_, getWidth(), getHeight(), -1, colorActual,
                      BND_LEFT, fontSize_, text_.c_str(), NULL);

    // Restore the original font face
    getWindow()->resetFontFace();
}

}  // namespace ui
} // namespace rack
