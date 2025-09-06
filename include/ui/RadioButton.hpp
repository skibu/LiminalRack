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
    void onDragStart(const DragStartEvent& e) override;
    void onDragEnd(const DragEndEvent& e) override;
    void onDragDrop(const DragDropEvent& e) override;
};

} // namespace ui
} // namespace rack
