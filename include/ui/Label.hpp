#pragma once
#include <widget/Widget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

class Label : public widget::Widget {
   public:
    /** Constructor with param for initialing text to a value. */
    Label(const std::string& text);

    /** Need default constructor since module libraries might have used it, like
     * for 4ms. Having default values for constructor parameters does not count
     * as a default constructor.
     */
    Label();

    enum Alignment {
        LEFT_ALIGNMENT,
        CENTER_ALIGNMENT,
        RIGHT_ALIGNMENT,
    };

    void setText(const std::string& text) {
        this->text_ = text;
    }

    /** Sets horizontal alignment for the text to leeft, center, or right */
    void setAlignment(Alignment alignment) {
        this->alignment_ = alignment;
    }

    /** Sets the color of the text. */
    void setColor(NVGcolor color) {
        this->color_ = color;
    }

    /** Sets the font size for the text of this label */
    void setFontSize(float fontSize) {
        this->fontSize_ = fontSize;
    }

    /** Gets the font size for the text of this label */
    float getFontSize() const {
        return fontSize_;
    }

    /** Sets the font face to use for this label in place of default one.
     * Should be a resource file path like "res/fonts/Roboto-Regular.ttf"
     */
    void setFontFaceOverride(const std::string& fontFile) {
        this->fontFaceOverride_ = fontFile;
    }

    /** Sets the line height for the text of this label */
    void setLineHeight(float lineHeight) {
        this->lineHeight_ = lineHeight;
    }

    /** To adjust y position to better align vertically */
    void setYOffset(float yOffset) {
        this->yOffset_ = yOffset;
    }

   private:
    std::string text_;
    NVGcolor color_;
    Alignment alignment_ = LEFT_ALIGNMENT;
    float yOffset_;
    float fontSize_;
    float lineHeight_;
    // For if want to use a different font face than the default
    std::string fontFaceOverride_ = "";

    void draw(const DrawArgs& args) override;
};

} // namespace ui
} // namespace rack
