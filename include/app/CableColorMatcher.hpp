#pragma once
#include <app/common.hpp>
#include <engine/PortInfo.hpp> // Add this include for PortInfo

namespace rack {
namespace app {

/**
 * CableColorMatcher is a utility class. It is for determinine appropriate
 * color for a cable based on the port names.
 */
class CableColorMatcher {
   public:
    /**
     * @brief Returns the color of the cable based on the port information.
     * If the ports do not have descriptive enough names to determine which color to use then
     * returns a default color. Looks at both ports in case names of indidual ports are not
     * descriptive enough.
     * @param portInfo1_p The first port information.
     * @param portInfo2_p The second port information, optional.
     * @return NVGcolor The color of the cable.
     */
    static NVGcolor getCableColor(engine::PortInfo* portInfo1_p,
                                  engine::PortInfo* portInfo2_p = NULL);
};

} // namespace app
} // namespace rack