#pragma once
#include <app/common.hpp>
#include <widget/OpaqueWidget.hpp>
#include <app/RackScrollWidget.hpp>
#include <app/RackWidget.hpp>
#include <app/SplashWidget.hpp>


namespace rack {
namespace app {

// Forward declaration of Browser class
namespace browser {
    class Browser;
}


/** The top level widget. Contains all other widgets in the application
 * including the Rack, MenuBar, Browser, etc.
 */
class Scene : public widget::OpaqueWidget {
   public:
    /** Constructor. Creates the top level widgets MenuBar, RackScrollWidget,
     * Splash Screen, and Browser */
    PRIVATE Scene();

    /** Destructor, frees up resources */
    PRIVATE ~Scene();

    /** Returns the current mouse position in the Scene's local coordinates */
    math::Vec getMousePos();

    /** Returns the menu bar widget that is contained by the Scene */
    widget::Widget* getMenuBar();

    /** Returns the rack widget that is contained by the RackScrollWidget,
     * which in turns is contained by the Scene.
     */
    RackWidget* getRack();

    /** Returns the rack scroll widget that is contained by the Scene */
    RackScrollWidget* getRackScroll();

    /** Returns the module browser widget that is contained by the Scene.
     * If want to show/hide the browser, use the overlay since the overlay
     * contains the browser.
     */
    browser::Browser* getBrowser() {
        return browser_;
    };

    /** Returns the overlay of the module browser widget that is contained by 
     * the Scene. If want to show/hide the browser, use this overlay since the 
     * overlay contains the browser..
     */
    widget::Widget *getBrowserOverlay() {
        return browserOverlay_;
    };

    // draw() and step() called directly so must be public
    /** Called once per frame to update the Scene */
    void step() override;

    /** Called once per frame to actually draw the Scene */
    void draw(const DrawArgs& args) override;

   private:
    struct Internal;
    Internal* internal_;

    // Convenience variables for accessing important widgets
    RackScrollWidget* rackScroll_;
    RackWidget* rack_;
    widget::Widget* menuBar_;
    browser::Browser* browser_;
    widget::Widget* browserOverlay_;
    SplashWidget* splashWidget_;

    // The last mouse position in the Scene's local coordinates
    math::Vec mousePos_;

    void onHover(const HoverEvent& e) override;
    void onDragHover(const DragHoverEvent& e) override;
    void onHoverKey(const HoverKeyEvent& e) override;
    void onPathDrop(const PathDropEvent& e) override;
};

} // namespace app
} // namespace rack
