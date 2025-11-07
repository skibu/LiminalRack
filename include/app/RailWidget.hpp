#pragma once
#include <app/common.hpp>
#include <widget/TransparentWidget.hpp>


namespace rack {
namespace app {

/** For drawing the back of the rack where there is no module covering it.
 * Includes the rails and also the power board.
 */
class RailWidget : public widget::TransparentWidget {
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
