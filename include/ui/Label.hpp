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

    void setText(const std::string& text) { this->text = text; }
    void setAlignment(Alignment alignment) { this->alignment = alignment; }
    void setColor(NVGcolor color) { this->color = color; }  
    void setFontSize(float fontSize) { this->fontSize = fontSize; }
    float getFontSize() const { return fontSize; }

    /** Sets the font face to use for this label in place of default one */
    void setFontFaceOverride(const std::string& fontFile) { this->fontFaceOverride = fontFile; }

   private:
    std::string text;
    NVGcolor color;
    Alignment alignment = LEFT_ALIGNMENT;
    float fontSize;
    float lineHeight;
    // For if want to use a different font face than the default
    std::string fontFaceOverride = "";

    void draw(const DrawArgs& args) override;
};

} // namespace ui
} // namespace rack
