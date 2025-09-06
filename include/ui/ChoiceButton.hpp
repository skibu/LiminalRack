#pragma once
#include <ui/common.hpp>
#include <ui/Button.hpp>


namespace rack {
namespace ui {


/** Button with a dropdown icon on its right.
*/
class ChoiceButton : public Button {
   public:
    ChoiceButton(const std::string& text = "") : Button(text) {}

   private:
    void draw(const DrawArgs& args) override;
};


} // namespace ui
} // namespace rack
