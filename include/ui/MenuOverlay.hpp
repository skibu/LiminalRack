#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

/**
 * Useful for covering up everything else and dimming it.
 * Deletes itself from parent when clicked. Used for menus,
 * but also other modal overlays like the module Browser window.
 */
class MenuOverlay : public widget::OpaqueWidget {
   public:
    /** Just basic setup. Cannot set size of the MenuOverlay in constructor
     * since parent not yet set. Size set in step() instead.
     */
    MenuOverlay();

    // Cleans up allocated memory since for menu overlays they are always
    // created on the heap (though that is not enforced).
    ~MenuOverlay();

    void setBgColor(NVGcolor color) { bgColor_ = color; }

    NVGcolor getBgColor() const { return bgColor_; }

    /** Sets size of the MenuOverlay to match its parent's size. This is
     * done in step() since in the constructor the parent isn't yet set because
     * parent.addChild() not called until after the object has been constructed.
     */
    void step() override;

   private:
    NVGcolor bgColor_;

   private:
    void draw(const DrawArgs& args) override;

    /** Catches all button events since MenuOverlays cover the entire
     * Scene. Consumes the button event. If a button press event then calls
     * onAction() to delete and close the menu overlay. */
    void onButton(const ButtonEvent& event) override;

    /** Consumes the event. If a key press event then calls onAction() to
     * delete and close the menu overlay if Escape key pressed. */
    void onHoverKey(const HoverKeyEvent& event) override;

    /** Closes and frees memory for the MenuOverlay and associated menus  */
    void onAction(const ActionEvent& event) override;
};

} // namespace ui
} // namespace rack
