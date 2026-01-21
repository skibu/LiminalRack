#pragma once
#include <ui/common.hpp>
#include <ui/Button.hpp>


namespace rack {
namespace ui {


/** Toggles a Quantity between 1.0 and 0.0 when clicked.
*/
class RadioButton : public Button {
   public:
    RadioButton(const std::string& text) : Button(text) {}

   private:
    void draw(const DrawArgs& args) override;

    /* @deprecated replaced by onButton()
    void onDragStart(const DragStartEvent& e) override;
    void onDragEnd(const DragEndEvent& e) override;
        */

    /** @deprecated replaced by onButton() but need to keep for linking*/
    void onDragDrop(const DragDropEvent& e) override;

    /** Handles button event.  Toggles state upon left click*/
    void onButton(const ButtonEvent& event) override;
};

} // namespace ui
} // namespace rack
