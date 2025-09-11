#pragma once
#include <ui/common.hpp>
#include <ui/Button.hpp>


namespace rack {
namespace ui {

/** Button with a dropdown icon on its right. The button allows user to select
 * from list of options. */
class ChoiceButton : public Button {
   public:
    ChoiceButton(const std::string& text = "") : Button(text) {}

   private:
    /** Uses beldnish bndChoiceButton() to actually draw the choice button */
    void draw(const DrawArgs& args) override;
};

} // namespace ui
} // namespace rack
