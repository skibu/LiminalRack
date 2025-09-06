#pragma once
#include <widget/Widget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

class Tooltip : public widget::Widget {
   public:
    Tooltip(const std::string& text) : text(text) {}
    Tooltip() {}

    /** Text to display in the tooltip */
    std::string text;

    /** Position and size the tooltip */
    void step() override;

    /** Actually draw the tooltip */
    void draw(const DrawArgs& args) override;
};

} // namespace ui
} // namespace rack
