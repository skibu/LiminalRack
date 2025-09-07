#include <app/Scene.hpp>
#include <context.hpp>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <ui/Tooltip.hpp>
#include <settings.hpp>

namespace rack {
namespace ui {

/** So can clamp tooltip width to half the window width */
static float maxTooltipWidth() {
    return settings::windowSize.x / 3.0; // Third the window width
}

void Tooltip::step() {
    // Save the current render state
    nvgSave(APP->window->vg);

    // Set line height to reasonable value
    nvgTextLineHeight(APP->window->vg, 1.2);

    // Set size of tooltip to fit contents
    box.size.x = std::min(maxTooltipWidth(), bndLabelWidthForFontSize(
        APP->window->vg, -1, settings::tooltipFontSize, text.c_str()));
    box.size.y = bndLabelHeightForFontSize(
        APP->window->vg, -1, settings::tooltipFontSize, text.c_str(), maxTooltipWidth());
    // add bit of vertical padding
    box.size.y += 2;

    // Position tooltip near cursor. This assumes that the Tooltip is added
    // to the root widget.
    math::Vec offset = settings::hasTouchscreen
                           ?
                           // For touchscreen don't want finger to cover tooltip
                           // so place it above finger
                           math::Vec(12, -12 - box.size.y)
                           :
                           // Default tooltip offset
                           math::Vec(15, 15);
    box.pos = APP->scene->mousePos.plus(offset);

    // Fit tooltop inside parent's box
    assert(parent);
    box = box.nudge(parent->box.zeroPos());

    // Restore the previous render state
    nvgRestore(APP->window->vg);

    Widget::step();
}

/** 
 * Helper function that converts any floating point numbers 
 * within. a string to have specified precision. This is useful
 * for formatting numbers in tooltips since code for a module might
 * have specified a silly amount of precision that is not useful.
 * For numbers >= 100 will use precision of 0, 
 * else for numbers >= 10 will use precision of 1,
 * else will use precision of 2. 
 * This way the precision will stay consistent as much as possible, yet be minimized.
 * @param inputString The string containing floating point numbers.
 * @return A new string with floating point numbers formatted to the desired precision.
 */
static std::string formatFloatingPoints(const std::string& inputString) {
    std::string result = inputString;
    // Regular expression to find floating-point numbers (e.g., 123.45, -0.789, .5)
    std::regex float_regex(R"([-+]?\d*\.+\d+([eE][-+]?\d+)?)"); 

    auto words_begin = std::sregex_iterator(result.begin(), result.end(), float_regex);
    auto words_end = std::sregex_iterator();

    std::vector<std::pair<std::string, std::string>> replacements;

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string original_float_str = match.str();
        
        try {
            double value = std::stod(original_float_str); // Convert to double
            std::ostringstream oss;
			// Convert to string with appropriate precision
            int desired_precision;
            if (std::abs(value) >= 100.0) {
                desired_precision = 0; // No decimal places for large numbers
            } else if (std::abs(value) >= 10.0) {
                desired_precision = 1; // One decimal place for medium numbers
            } else {
                desired_precision = 2;  // else, use two decimal places for small numbers
            }
            oss << std::fixed << std::setprecision(desired_precision) << value;
            replacements.push_back({original_float_str, oss.str()});
        } catch (const std::invalid_argument& e) {
            // Not a valid number, skip
        } catch (const std::out_of_range& e) {
            // Number out of range, skip
        }
    }

    // Perform replacements from end to beginning to avoid iterator invalidation issues
    for (int i = replacements.size() - 1; i >= 0; --i) {
        size_t pos = result.find(replacements[i].first);
        if (pos != std::string::npos) {
            result.replace(pos, replacements[i].first.length(), replacements[i].second);
        }
    }

    return result;
}

void Tooltip::draw(const DrawArgs& args) {
    bndTooltipBackground(args.vg, 0.0, 0.0, box.size.x, box.size.y);
    nvgTextLineHeight(args.vg, 1.2);

    // Because there is no bndThemeLabel() function, temporarily replace the
    // menu text color with tooltip text color and draw a menu label
    BNDtheme* theme = bndGetTheme();
    NVGcolor menuTextColor = theme->menuTheme.textColor;
    theme->menuTheme.textColor = theme->tooltipTheme.textColor;

    // Format floating point numbers
    text = formatFloatingPoints(text);
    bndTooltipLabel(args.vg, 0.0, 0.0, maxTooltipWidth(), box.size.y,
                    settings::tooltipFontSize, text.c_str());
    theme->menuTheme.textColor = menuTextColor;

    Widget::draw(args);
}

} // namespace ui
} // namespace rack
