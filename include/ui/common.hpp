/**
 * For managing colors of the various UI components. 
 */

#pragma once
#include <nanovg.h>
#include <blendish.h>

#include <common.hpp>
#include <color.hpp>

/** Useful for menu items with a "true" boolean state */
#define CHECKMARK_STRING "✔"
#define CHECKMARK(_cond) ((_cond) ? CHECKMARK_STRING : "")

/** Useful for menu items that open a sub-menu */
#define RIGHT_ARROW "▸"


namespace rack {


/** Common graphical user interface widgets */
namespace ui {


PRIVATE void init();
PRIVATE void destroy();

/**
 * @brief Sets the theme colors using the colors specified by bg and fg params.
 * 
 * @param bg background color
 * @param fg  foreground color
 */
PRIVATE void setTheme(NVGcolor bg, NVGcolor fg);

/** Sets theme colors using value of settings::uiTheme which can be set to 
 * "light", "dark", or "hcdark". Sets the colors associated with the
 * theme. */
void refreshTheme();


} // namespace ui
} // namespace rack
