#include <gamepad.hpp>
#include <midi.hpp>
#include <string.hpp>
#include <window/Window.hpp>


namespace rack {
namespace gamepad {

// Forward declaration
class GamepadDriver;


static const int DRIVER = -10;
static GamepadDriver* driver_s = nullptr;


class GamepadInputDevice : public midi::InputDevice {
    public:
    void setDeviceId(int deviceId) {
        deviceId_ = deviceId;
    }

	int deviceId_;
	int16_t ccValues_[128] = {};

	std::string getName() override {
		const char* name = glfwGetJoystickName(deviceId_);
		if (!name)
			return "";
		return name;
	}

	void step() {
		if (!glfwJoystickPresent(deviceId_))
			return;

		// Get gamepad state
		int numAxes;
		const float* axes = glfwGetJoystickAxes(deviceId_, &numAxes);
		int numButtons;
		const unsigned char* buttons = glfwGetJoystickButtons(deviceId_, &numButtons);

		// Convert axes and buttons to MIDI CC
		// Unfortunately to support 14-bit MIDI CC, only the first 32 CCs can be used.
		// This could be fixed by continuing with CC 64 if more than 32 CCs are needed.
		int numCcs = std::min(numAxes + numButtons, 32);
		for (int i = 0; i < numCcs; i++) {
			// Allow CC value to go negative
			int16_t value;
			if (i < numAxes) {
				// Axis
				value = math::clamp((int) std::round(axes[i] * 0x3f80), -0x3f80, 0x3f80);
			}
			else {
				// Button
				value = buttons[i - numAxes] ? 0x3f80 : 0;
			}

			if (value == ccValues_[i])
				continue;
			ccValues_[i] = value;

			// Send MSB MIDI message
			midi::Message msg;
			msg.setStatus(0xb);
			msg.setNote(i);
			// Allow 8th bit to be set to allow bipolar value hack.
			msg.bytes[2] = (value >> 7);
			onMessage(msg);

			// Send LSB MIDI message for axis CCs
			if (i < numAxes) {
				midi::Message msg;
				msg.setStatus(0xb);
				msg.setNote(i + 32);
				msg.bytes[2] = (value & 0x7f);
				onMessage(msg);
			}
		}
	}
};


class GamepadDriver : public midi::Driver {
    public:
	GamepadDriver() {
		for (int i = 0; i < 16; i++) {
			devices_[i].setDeviceId(i);
		}
	}
    
    GamepadInputDevice& getGamepadDevice(int i) {
        return devices_[i];
    }

	std::string getName() override {
		return "Gamepad";
	}

    private:
	GamepadInputDevice devices_[16];

	std::vector<int> getInputDeviceIds() override {
		std::vector<int> deviceIds;
		for (int i = 0; i < 16; i++) {
			if (glfwJoystickPresent(i)) {
				deviceIds.push_back(i);
			}
		}
		return deviceIds;
	}

	int getDefaultInputDeviceId() override {
		return 0;
	}

	std::string getInputDeviceName(int deviceId) override {
		if (!(0 <= deviceId && deviceId < 16))
			return "";

		const char* name = glfwGetJoystickName(deviceId);
		if (!name)
			return string::f("#%d (unavailable)", deviceId + 1);
		return name;
	}

	midi::InputDevice* subscribeInput(int deviceId, midi::Input* input) override {
		if (!(0 <= deviceId && deviceId < 16))
			return NULL;
		if (!glfwJoystickPresent(deviceId))
			return NULL;

        GamepadInputDevice& dev = getGamepadDevice(deviceId);
		dev.subscribe(input);
		return &dev;
	}

	void unsubscribeInput(int deviceId, midi::Input* input) override {
		if (!(0 <= deviceId && deviceId < 16))
			return;

		getGamepadDevice(deviceId).unsubscribe(input);
	}
};




void init() {
    // If already initialized, return
    if (driver_s) return;

	driver_s = new GamepadDriver();
	midi::addDriver(DRIVER, driver_s);
}

void step() {
    // Initialize driver if hasn't been done yet
    if (!driver_s) init();

    // Step all connected gamepads
	for (int i = 0; i < 16; i++) {
		if (glfwJoystickPresent(i)) {
			driver_s->getGamepadDevice(i).step();
		}
	}
}


} // namespace gamepad
} // namespace rack
