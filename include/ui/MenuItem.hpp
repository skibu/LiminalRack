#pragma once
#include <ui/common.hpp>
#include <ui/Menu.hpp>
#include <ui/MenuEntry.hpp>
#include <context.hpp>


namespace rack {
namespace ui {

/** 
 * A selectable label that can be an item in a menu 
 */
class MenuItem : public MenuEntry {
   public:
    MenuItem(const std::string& text = "", const std::string& name = "") : MenuEntry(name), text(text) {}

    /** Destructor */
    ~MenuItem() override {
        TRACE("~MenuItem() called for %s", getName().c_str());
    }

    /** Sets the text of the menu item */
    void setText(const std::string& text) {
        this->text = text;
    }

    /** Returns the text of the menu item */
    const std::string& getText() const {
        return text;
    }

    void setRightText(const std::string& rightText) {
        this->rightText = rightText;
    }

    bool isDisabled() const {
        return disabled;
    }

    void setDisabled(bool disabled) {
        this->disabled = disabled;
    }

    /** So that other classes can create an action */
    void doAction(bool consume = true);

   protected:
    /**
     * Actually draws this menu item, usimg the specified x-offset
     *
     * @param vg graphics manager
     * @param offset the x offset (indent) for drawing the label. Default is 0.
     * Used for ColorDotMenuItem
     */
    PRIVATE void drawOffset(NVGcontext* vg, float offset = 0);
    void step() override;

   private:
    std::string text;
    std::string rightText;
    bool disabled = false;

    /** Draws menu item using an x offset of 0 */
    void draw(const DrawArgs& args) override;

    void onEnter(const EnterEvent& e) override;

    /** @deprecated replaced by onButton() but have to keep for ABI compatibility
     */
    void onDragDrop(const DragDropEvent& e) override;
    

    /** Handles button event.  Calls doAction() which pops up menu on left click
     */
    void onButton(const ButtonEvent& event) override;

    /** Override to create a child menu when this menu item is hovered over.
     * Return nullptr if no child menu.
     */
    virtual Menu* createChildMenu() {
        return nullptr;
    }

    /** Override to handle behavior when user clicks the menu item.
     * Event is consumed by default. Unconsume to prevent the menu from being
     * closed. If Ctrl (Cmd on Mac) is held, the event is *not* pre-consumed, so
     * if your menu must be closed, always consume the event.
     */
    void onAction(const ActionEvent& e) override;
};

/**
 * A MenuItem, but with a colored dot on the left before the label.
 * Useful for things like choosing cable color where want to display
 * the color.
 */
struct ColorDotMenuItem : MenuItem {
    ColorDotMenuItem(const std::string& text = "", const std::string& name = "")
        : MenuItem(text, name) {}
        
    NVGcolor color = color::BLACK_TRANSPARENT;

    void draw(const DrawArgs& args) override;
    void step() override;
};

} // namespace ui
} // namespace rack
