/** 
 * The Browser is the window that shows the modules the user has
 * available and can add to their rack.
 */

#pragma once
#include <app/common.hpp>
#include <widget/Widget.hpp>


namespace rack {
namespace app {

/** Initializes the browser */
PRIVATE void browserInit();

/** Creates the browser window */
PRIVATE widget::Widget* browserCreate();


} // namespace app
} // namespace rack
