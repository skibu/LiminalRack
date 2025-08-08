#include <app/Scene.hpp>
#include <context.hpp>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <ui/Tooltip.hpp>

namespace rack {
namespace ui {


void Tooltip::step() {
	// Wrap size to contents
	nvgSave(APP->window->vg);
	nvgTextLineHeight(APP->window->vg, 1.2);
	box.size.x = bndLabelWidth(APP->window->vg, -1, text.c_str());
	box.size.y = bndLabelHeight(APP->window->vg, -1, text.c_str(), INFINITY);
	// Position near cursor. This assumes that the Tooltip is added to the root widget.
	box.pos = APP->scene->mousePos.plus(math::Vec(15, 15));
	// Fit inside parent
	assert(parent);
	box = box.nudge(parent->box.zeroPos());
	nvgRestore(APP->window->vg);

	Widget::step();
}

/** 
 * Helper function that converts any floating point numbers 
 * within. a string to have specified precision. This is useful
 * for formatting numbers in tooltips since code for a module might
 * have specified a silly amount of precision that is not useful.
 */
static std::string formatFloatingPoints(const std::string& inputString, int precision = 2) {
    std::string result = inputString;
    // Regular expression to find floating-point numbers (e.g., 123.45, -0.789, .5)
    std::regex float_regex(R"([-+]?\d*\.?\d+([eE][-+]?\d+)?)"); 

    auto words_begin = std::sregex_iterator(result.begin(), result.end(), float_regex);
    auto words_end = std::sregex_iterator();

    std::vector<std::pair<std::string, std::string>> replacements;

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string original_float_str = match.str();
        
        try {
            double value = std::stod(original_float_str); // Convert to double
            std::ostringstream oss;
			// Convert to string with specified precision
            oss << std::fixed << std::setprecision(precision) << value;  
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

	// Because there is no bndThemeLabel() function, temporarily replace the menu text color with tooltip text color and draw a menu label
	BNDtheme* theme = (BNDtheme*) bndGetTheme();
	NVGcolor menuTextColor = theme->menuTheme.textColor;
	theme->menuTheme.textColor = theme->tooltipTheme.textColor;
	// Format floating points with 2 decimal places
	text = formatFloatingPoints(text, 2); 
	bndMenuLabel(args.vg, 0.0, 0.0, INFINITY, box.size.y, -1, text.c_str());
	theme->menuTheme.textColor = menuTextColor;

	Widget::draw(args);
}


} // namespace ui
} // namespace rack
