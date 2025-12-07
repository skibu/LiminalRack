#pragma once
#include <common.hpp>


namespace rack {
/** Gamepad/joystick/controller MIDI driver */
namespace gamepad {

/** Initialize the gamepad driver. Called by step() so don't need to call
 * explicitly */
PRIVATE void init();

/** Called every frame */
PRIVATE void step();


} // namespace gamepad
} // namespace rack
