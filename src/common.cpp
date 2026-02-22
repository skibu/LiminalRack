#include <common.hpp>
#include <string.hpp>
#include <GLFW/glfw3.h>

#if defined ARCH_WIN
#include <windows.h>

FILE* fopen_u8(const char* filename, const char* mode) {
	return _wfopen(rack::string::UTF8toUTF16(filename).c_str(), rack::string::UTF8toUTF16(mode).c_str());
}

#endif


namespace rack {


const std::string APP_NAME = "Rack";
const std::string APP_EDITION = "Free";
const std::string APP_EDITION_NAME = "";
const std::string APP_VERSION_MAJOR = "2";
const std::string APP_VERSION = TOSTRING(_RACK_VERSION);
#if defined ARCH_WIN
	const std::string APP_OS = "win";
	const std::string APP_OS_NAME = "Windows";
#elif defined ARCH_MAC
	const std::string APP_OS = "mac";
	const std::string APP_OS_NAME = "macOS";
#elif defined ARCH_LIN
	const std::string APP_OS = "lin";
	const std::string APP_OS_NAME = "Linux";
#endif
#if defined ARCH_X64
	const std::string APP_CPU = "x64";
	const std::string APP_CPU_NAME = "x64";
#elif defined ARCH_ARM64
	const std::string APP_CPU = "arm64";
	const std::string APP_CPU_NAME = "ARM64";
#endif
const std::string API_URL = "https://api.vcvrack.com";


Exception::Exception(const char* format, ...) {
	va_list args;
	va_start(args, format);
	msg = string::fV(format, args);
	va_end(args);
}

bool isWindows() { return APP_OS == "win"; }
bool isMac() { return APP_OS == "mac"; }
bool isLinux() { return APP_OS == "lin"; }
bool isX64() { return APP_CPU == "x64"; }
bool isArm64() { return APP_CPU == "arm64"; }

bool isWayland() {
    // Note: glfwGetPlatform() can only return proper platform once GLFW is
    // inialized. Before that need to use isLinux() as a proxy for Wayland since
    // Wayland is only used on Linux. But after GLFW is initialized then can use
    // glfwGetPlatform() to check if Wayland is being used since some Linux
    // machines use X11 instead of Wayland.
    auto platform = glfwGetPlatform();
    if (platform != 0) { 
        // GLFW initialized so platform is valid
        return platform == GLFW_PLATFORM_WAYLAND;
    } else {
        // GLFW not yet initialized so using isLinux() as a proxy for Wayland
        return isLinux();
    }
}

} // namespace rack 
