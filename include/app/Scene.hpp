#pragma once
#include <app/common.hpp>
#include <widget/OpaqueWidget.hpp>
#include <app/RackScrollWidget.hpp>
#include <app/RackWidget.hpp>


namespace rack {
namespace app {

class Scene : public widget::OpaqueWidget {
   public:
    PRIVATE Scene();
    PRIVATE ~Scene();
    math::Vec getMousePos();
    widget::Widget* getMenuBar();
    RackWidget* getRack();
    RackScrollWidget* getRackScroll();
    widget::Widget* getBrowser();

    // draw() and step() called directly so must be public
    void draw(const DrawArgs& args) override;
    void step() override;

   private:
    struct Internal;
    Internal* internal_;

    // Convenience variables for accessing important widgets
    RackScrollWidget* rackScroll;
    RackWidget* rack;
    widget::Widget* menuBar;
    widget::Widget* browser;

    // The last mouse position in the Scene's local coordinates
    math::Vec mousePos;

    void onHover(const HoverEvent& e) override;
    void onDragHover(const DragHoverEvent& e) override;
    void onHoverKey(const HoverKeyEvent& e) override;
    void onPathDrop(const PathDropEvent& e) override;
};

} // namespace app
} // namespace rack
