#pragma once
#include <vector>
#include <set>
#include <map>
#include <list>
#include <tuple>

#include <jansson.h>

#include <common.hpp>
#include <math.hpp>
#include <color.hpp>


namespace rack {
/** Process-scope globals, most of which are persisted across launches */
namespace settings {


// Runtime state, not serialized.

/** Path to settings.json */
extern std::string settingsPath;
extern bool devMode;
extern bool headless;
extern bool isPlugin;
/** Requests to restart the application on exit. */
extern bool restart;

// Persistent state, serialized to settings.json.

/** ISO 639-1 language code for string translations. */
extern std::string language;
/** To customize the Module Browser window background */
extern NVGcolor moduleBrowserBg;

/** Background color for light mode theme */
extern NVGcolor lightModeThemeBg;
/** Foreground color for light mode theme */
extern NVGcolor lightModeThemeFg;

/** Background color for dark mode theme */
extern NVGcolor darkModeThemeBg;
/** Foreground color for dark mode theme */
extern NVGcolor darkModeThemeFg;

/** Background color for tooltips. Want them to be different from other
 * interactive items */
extern NVGcolor tooltipBg;
/** Foreground color for tooltips */
extern NVGcolor tooltipFg;
/** Font size for tooltips */
extern int tooltipFontSize;

/** Color for when selecting modules. Not stored in json file. */
extern NVGcolor selectModuleFillColor;

/** Color for when selecting modules. Not stored in json file. */
extern NVGcolor selectModuleStrokeColor;

/** Whether the window is maximized */
extern bool windowMaximized;

/** True if a fork of VCV Rack and things need to be done differently */
extern bool isNotVCVRack;
/** Whether this is Liminal version of Rack */
extern bool isLiminal;
/** Whether touchscreen being used */
extern bool hasTouchscreen;
/** Whether keyboard being used */
extern bool hasKeyboard;
/** Font file to use for UI */
extern std::string systemFontFileName;
/** Monospaced font file to use for UI */
extern std::string systemMonospacedFontFileName;
/** Font size to use for blendish */
extern int bndLabelFontSize;
/** Height of widgets in pixels */
extern int bndWidgetHeight;
/** Size of window in pixels */
extern math::Vec windowSize;
/** Position in window in pixels */
extern math::Vec windowPos;
/** Reverse the zoom scroll direction */
extern bool invertZoom;
/** Mouse wheel zooms instead of pans. */
extern bool mouseWheelZoom;
/** Ratio between UI pixel and physical screen pixel. 0 for auto. */
extern float pixelRatio;
/** Name of UI theme, specified in ui::refreshTheme() */
extern std::string uiTheme;
/** Opacity of cables in the range [0, 1] */
extern float cableOpacity;
/** Straightness of cables in the range [0, 1]. Unitless and arbitrary. */
extern float cableTension;
/** Random addition to straightness of cables in the range [0, 0.3]. Unitless and arbitrary. */
extern float cableTensionRandomFactor;
/** Sometimes nice to dim rack so that it isn't too bright */
extern float rackBrightness;
/** When dimming rack it can be nice to have system draw halos around the lights
 * on the module */
extern float haloBrightness;
/** Allows rack to hide and lock the cursor position when dragging knobs etc. */
extern bool allowCursorLock;
enum KnobMode {
	KNOB_MODE_LINEAR,
	KNOB_MODE_SCALED_LINEAR,
	KNOB_MODE_ROTARY_ABSOLUTE,
	KNOB_MODE_ROTARY_RELATIVE,
};

/** How the user can interact with the knobs */
extern KnobMode knobMode;
extern bool knobScroll;
extern float knobLinearSensitivity;
extern float knobScrollSensitivity;

/** Audio sample rate */
extern float sampleRate;
/** Number of threads to use. The more threads the more audio can be processed.
 * But if you specify that all cores should be used then the computer might get
 * bogged down.*/
extern int threadCount;
/** Whether tooltips are enabled  */
extern bool tooltips;
/** Whether knob shadows are shown */
extern bool showKnobShadows;
/** Whether CPU meter is enabled */
extern bool cpuMeter;
/** Don't allow user to drag modules around */
extern bool lockModules;
/** Whether user allowed to drag modules in between other modules even 
 * if there isn't a sufficent gap. */
extern bool squeezeModules;
/** Uses dark panels if they are available */
extern bool preferDarkPanels;
/** Maximum screen redraw frequency in Hz, or 0 for unlimited. */
extern float frameRateLimit;
/** Interval between autosaves in seconds. */
extern float autosaveInterval;
/** Launches Rack without loading plugins or the autosave patch. Always set to false when settings are saved. */
extern bool safeMode;
/** vcvrack.com user token */
extern std::string token;
extern bool skipLoadOnLaunch;
extern std::string lastPatchDirectory;
extern std::string lastSelectionDirectory;
extern std::list<std::string> recentPatchPaths;
extern std::vector<NVGcolor> cableColors;
extern std::vector<std::string> cableLabels;
extern bool cableAutoRotate;
extern bool autoCheckUpdates;
extern bool verifyHttpsCerts;
extern bool showTipsOnLaunch;
extern int tipIndex;
enum BrowserSort {
	BROWSER_SORT_UPDATED,
	BROWSER_SORT_LAST_USED,
	BROWSER_SORT_MOST_USED,
	BROWSER_SORT_BRAND,
	BROWSER_SORT_NAME,
	BROWSER_SORT_RANDOM,
};
extern BrowserSort browserSort;
extern float browserZoom;
extern json_t* pluginSettingsJ;

struct ModuleInfo {
	bool enabled = true;
	bool favorite = false;
	int added = 0;
	double lastAdded = NAN;
};
/** pluginSlug -> (moduleSlug -> ModuleInfo) */
extern std::map<std::string, std::map<std::string, ModuleInfo>> moduleInfos;
/** Returns a ModuleInfo if exists for the given slugs.
*/
ModuleInfo* getModuleInfo(const std::string& pluginSlug, const std::string& moduleSlug);

/** The VCV JSON API returns the data structure
{pluginSlug: [moduleSlugs] or true}
where "true" represents that the user is subscribed to the plugin (all modules and future modules).
C++ isn't weakly typed, so we need the PluginWhitelist data structure to store this information.
*/
struct PluginWhitelist {
	bool subscribed = false;
	std::set<std::string> moduleSlugs;
};
extern std::map<std::string, PluginWhitelist> moduleWhitelist;

bool isModuleWhitelisted(const std::string& pluginSlug, const std::string& moduleSlug);

/**
 * @brief Sets cableColors and cableLabels to default values
 *  Then they can be overridden when settings.json file read in.
 */
void resetCables();

PRIVATE void init();
PRIVATE void destroy();

/** Converts settings to JSON object */
PRIVATE json_t* toJson();

/** Converts JSON object to settings */
PRIVATE void fromJson(json_t* rootJ);

/** Saves settings to file, in JSON format */
PRIVATE void save(std::string path = "");

/** Loads settings from file, in JSON format */
PRIVATE void load(std::string path = "");

/**
 * There are two labelFontSize variables, the blendish one and the settings one.
 * Both are used in the Rack code, somewhat randomly. Therefore need to
 * use this setter in order to make sure they are synced. Therefore should 
 * use this function to set the value.
 */
PRIVATE void setLabelFontSize(int size);
/** Gets current value of the settings labelFontSize */
PRIVATE int getLabelFontSize();

/**
 * There are two widgetHeight variables, the blendish one and the settings one.
 * Both are used in the Rack code, somewhat randomly. Therefore need to
 * use this setter in order to make sure they are synced. Therefore should 
 * use this function to set the value.
 */
PRIVATE void setWidgetHeight(int height);
/** Gets current value of the settings widgetHeight */
PRIVATE int getWidgetHeight();

PRIVATE void initBlendish();

} // namespace settings
} // namespace rack
