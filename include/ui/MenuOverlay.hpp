#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

/**
 * Useful for covering up everything else and dimming it.
 * Deletes itself from parent when clicked.
 */
class MenuOverlay : public widget::OpaqueWidget {
   public:
    MenuOverlay();

    // Cleans up allocated memory since for menu overlays they are always
    // created on the heap (though that is not enforced).
    ~MenuOverlay();

    void setBgColor(NVGcolor color) { bgColor_ = color; }

    NVGcolor getBgColor() const { return bgColor_; }

    void step() override;

   private:
    NVGcolor bgColor_;

   private:
    void draw(const DrawArgs& args) override;

    /** Consumes the event. If a button press event then calls onAction() to
     * delete and close the menu overlay. */
    void onButton(const ButtonEvent& e) override;

    /** Consumes the event. If a key press event then calls onAction() to
     * delete and close the menu overlay if Escape key pressed. */
    void onHoverKey(const HoverKeyEvent& e) override;

    /** Closes and frees memory for the MenuOverlay and associated menus  */
    void onAction(const ActionEvent& e) override;
};

} // namespace ui
} // namespace rack
