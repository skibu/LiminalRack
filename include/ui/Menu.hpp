#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>
#include <ui/MenuEntry.hpp>


namespace rack {
namespace ui {

class Menu : public widget::OpaqueWidget {
   public:
    Menu();

    /** Destructor. Cleans up child menus and entries */
    ~Menu();

    void setChildMenu(Menu* menu);

    /** Sets the corner flags for the menu background */
    void setCornerFlags(BNDcornerFlags cornerFlags) {
        this->cornerFlags_ = cornerFlags;
    }

    void setActiveEntry(MenuEntry* entry) { this->activeEntry_ = entry; }

    MenuEntry* getActiveEntry() { return activeEntry_; }

    void step() override;

   private:
    void draw(const DrawArgs& args) override;
    void onHoverScroll(const HoverScrollEvent& e) override;

   public:
    // parentMenu not used and deprecated, but kept for ABI compatibility.
    // Made public so don't get compiler warnings about unused private member.
    Menu* parentMenu = nullptr;

   private:
    Menu* childMenu_ = nullptr;
    /** The entry which created the child menu */
    MenuEntry* activeEntry_ = nullptr;
    BNDcornerFlags cornerFlags_ = BND_CORNER_NONE;
};

} // namespace ui
} // namespace rack
