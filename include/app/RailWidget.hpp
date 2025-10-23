#pragma once
#include <app/common.hpp>
#include <widget/TransparentWidget.hpp>


namespace rack {
namespace app {

struct RailWidget : widget::TransparentWidget {
   private:
    struct Internal;
    Internal* internal_;

   public:
    RailWidget();
    ~RailWidget();
    void step() override;
    void draw(const DrawArgs& args) override;
};

} // namespace app
} // namespace rack
