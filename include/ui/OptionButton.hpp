#pragma once
#include <ui/common.hpp>
#include <ui/RadioButton.hpp>


namespace rack {
namespace ui {

/** Behaves like a RadioButton and appears with a checkmark beside text.
 */
class OptionButton : public RadioButton {
   public:
    OptionButton(const std::string& text) : RadioButton(text) {}

   private:
    void draw(const DrawArgs& args) override;
};

} // namespace ui
} // namespace rack
