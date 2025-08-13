#include "app/CableColorMatcher.hpp"

#include <engine/Module.hpp>
#include <iterator>  // For std::begin and std::end
#include <logger.hpp>
#include <settings.hpp>
#include <string>

namespace rack {
namespace app {

static std::vector<std::string> audioNames = {"audio", "left", "right"};

static std::vector<std::string> pitchNames = {
    "pitch",
    "1v/oct",
};

static std::vector<std::string> cvNames = {
    "modulation",
    "cv",
    "lfo",
    "envelope",
};

static std::vector<std::string> gateNames = {"trigger", "gate", "clock", "reset"};

/**
 *  Returns a lowercase version of the given string
 */ 
static std::string toLower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower_str;
}

/**
 * Function to check if a substring is contained within a string. Is case-insensitive.
 */
static bool contains(const std::string& substring, const std::string& mainString) {
    return toLower(mainString).find(toLower(substring)) != std::string::npos;
}


/** 
 * Index into cable vector to the cable that indicates that no match could be found.
 * Returns the last cable in the vector.
 */
static size_t cableIndexWhenNoMatch() {
    return settings::cableLabels.size() - 1;
}


 /**
  * Checks if one of the strings in the vector is in the full_str.
  * This is case-insensitive. Useful for determining which port type
  * keywords match to the port label.
  */
static bool vectorStrInFullStr(const std::vector<std::string>& vec, const std::string& full_str) {
    // Sees if search_for_str found in any of the strings in the vector
    auto it = std::find_if(vec.begin(), vec.end(), [&](const std::string& string_from_vector) {
        return contains(string_from_vector, full_str);
    });

    // Return true if one of the strings in the vector is in the full_str
    return it != vec.end();
}


/**
 * Returns index of matching cable type
 */
static size_t getCableIndex(const std::string& cableType) {
    DEBUG("Searching for cable type: %s", cableType.c_str());
    for (size_t i = 0; i < settings::cableLabels.size(); ++i) {
        DEBUG("Checking for cableType=%s in cable label: %s", cableType.c_str(),
              settings::cableLabels[i].c_str());
        if (contains(cableType, settings::cableLabels[i])) {
            return i;
        }
    }

    // Did not find cable with desired type so returning color for last cable in
    // vector
    DEBUG("No cable type match found for: %s, returning last cable index: %ld",
         cableType.c_str(), cableIndexWhenNoMatch());
    return cableIndexWhenNoMatch();
}

/**
 * Finds where port label matches one of the defined cable types.
 * Returns the index into the cable types vector of the match. If
 * match cannot be found then returns cableIndexWhenNoMatch().
 */
static size_t match(std::string portLabel) {
    // Find where port label matches one of the defined cable types.
    // The order of checking is important since some port labels may match
    // multiple cable types, like "Gate select CV" which matches both
    // "cv" and "gate". We want CV to be matched first since it is more
    // general than gate, and we want to use the most general cable type
    // that matches the port label.
    if (vectorStrInFullStr(cvNames, portLabel)) {
        DEBUG("Port label matched to CV / Modulate for label: %s",
             portLabel.c_str());
        return getCableIndex("cv");
    } else if (vectorStrInFullStr(gateNames, portLabel)) {
        DEBUG("Port label matched to Gate / Trigger / Clock for label: %s",
              portLabel.c_str());
        return getCableIndex("gate");
    } else if (vectorStrInFullStr(pitchNames, portLabel)) {
        DEBUG("Port label matched to Pitch / 1V/Oct for label: %s",
             portLabel.c_str());
        return getCableIndex("pitch");
    } else if (vectorStrInFullStr(audioNames, portLabel)) {
        DEBUG("Port label matched to Audio for label: %s", portLabel.c_str());
        return getCableIndex("audio");
    } else {
        DEBUG("Port label not matched for label: %s", portLabel.c_str());
        // Not one of the defined cable types so use last cable in vector
        return cableIndexWhenNoMatch();
    }
}

/**
 * Returns vector of the tag names for the module.
 * This way can identify functionality of the module, such as filter or oscillator.
 */
static std::vector<std::string> getTagNames(const rack::engine::Module* module) {
    return module->model->getTagNames();
}

NVGcolor CableColorMatcher::getCableColor(engine::PortInfo* portInfo1,
                                          engine::PortInfo* portInfo2) {
    // Use last cable in vector as default
    size_t no_match_index = cableIndexWhenNoMatch();
    size_t cable_index = no_match_index;

    if (portInfo1) {
        const rack::plugin::Model* model = portInfo1->getModule()->model;
        auto brand = model->plugin->getBrand();
        std::string moduleName = model->name;
        DEBUG("Getting cable color for port portInfo1 name=%s brand=%s model=%s",
              portInfo1->getName().c_str(), brand.c_str(), moduleName.c_str());

        // Get best match using single port name
        cable_index = match(portInfo1->getName());
        DEBUG("Used first port name: %s to determine cable color. Cable index=%ld",
              portInfo1->getName().c_str(), cable_index);
    }

    if (portInfo2) {
        rack::plugin::Model* model = portInfo2->getModule()->model;
        auto brand = model->plugin->getBrand();
        std::string moduleName = model->name;
        DEBUG("Getting cable color for port portInfo2 name=%s brand=%s model=%s",
              portInfo2->getName().c_str(), brand.c_str(), moduleName.c_str());

        if (cable_index == no_match_index) {
            /* If no cable match found using first port label then use second port
            label to determine cable color. This is useful if the first port name
            is not descriptive enough. For example, if the first port is
            "mysterious" such that there is no match, and the second port is
            "Pitch" then we want to use the second port to determine the cable
            color. */
            cable_index = match(portInfo2->getName());
            if (cable_index != no_match_index) {
                DEBUG("Used second port name: %s to determine cable color. Cable index=%ld",
                      portInfo2->getName().c_str(), cable_index);
            }
        }
    }

    // If have connected cable to two ports but still cannot determine
    // cable color then try using the module type to determine the cable color.
    if (portInfo1 && portInfo2 && cable_index == no_match_index) {
        DEBUG(
            "Could not use second port names to determine cable color. "
            "Therefore seeing if can use module type for the output port.");

        // For output port then module type might be helpful. For example, a filter
        // output is going to be audio.
        auto outputPortModuleTagNames = portInfo1->type == engine::Port::OUTPUT ?
            getTagNames(portInfo1->getModule()) :
            getTagNames(portInfo2->getModule());

        if (vectorStrInFullStr(outputPortModuleTagNames, "oscillator") ) {
            DEBUG("Output port is from an oscillator so most likely audio output");
            cable_index = match("audio");
        } else if (vectorStrInFullStr(outputPortModuleTagNames, "filter")) {
            DEBUG("Output port is from an filter so most likely audio output");
            cable_index = match("audio");
        } else if (vectorStrInFullStr(outputPortModuleTagNames, "wavetable")) {
            DEBUG("Output port is from an wavetable so most likely audio output");
            cable_index = match("audio");
        }
    }

    // Convert the index to a cable color
    NVGcolor color = settings::cableColors[cable_index];

    DEBUG("Cable index=%ld Cable color r=%d g=%d b=%d a=%d", cable_index,
         (int)(color.r * 255), (int)(color.g * 255), (int)(color.b * 255),
         (int)(color.a * 255));
    return color;
}

}  // namespace app
}  // namespace rack