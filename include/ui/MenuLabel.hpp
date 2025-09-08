#pragma once
#include <ui/common.hpp>
#include <ui/MenuEntry.hpp>


namespace rack {
namespace ui {

/** A non-selectable label that can be an item in a menu */
class MenuLabel : public MenuEntry {
   public:
    MenuLabel(std::string textRef = "") : text(textRef) {}

    void setText(const std::string& text) {
        this->text = text;
    }

   protected:
    void step() override;

   private:
    std::string text;

    void draw(const DrawArgs& args) override;
};

} // namespace ui
} // namespace rack
