/**
 * @file standalone.cpp
 *
 * Contains the application's main() function. Handles the startup
 * of the application. Everything from processing command line options,
 * loading config settings, initializing modules, and then running the
 * main loop.
 */

#include <ui/liminal.hpp>

#include <common.hpp>
#include <random.hpp>
#include <asset.hpp>
#include <audio.hpp>
#include <rtaudio.hpp>
#include <midi.hpp>
#include <rtmidi.hpp>
#include <keyboard.hpp>
#include <gamepad.hpp>
#include <midiloopback.hpp>
#include <settings.hpp>
#include <engine/Engine.hpp>
#include <app/common.hpp>
#include <app/Scene.hpp>
#include <app/SplashWidget.hpp>
#include <app/Browser.hpp>
#include <plugin.hpp>
#include <context.hpp>
#include <window/Window.hpp>
#include <patch.hpp>
#include <history.hpp>
#include <ui/common.hpp>
#include <system.hpp>
#include <string.hpp>
#include <library.hpp>
#include <network.hpp>

#include <getopt.h>
#include <unistd.h> // for getopt
#include <signal.h> // for signal
#if defined ARCH_WIN
	#include <windows.h> // for CreateMutex
#endif
#include <osdialog.h>

#if defined ARCH_MAC
	#define GLFW_EXPOSE_NATIVE_COCOA
	#include <GLFW/glfw3native.h> // for glfwGetOpenedFilenames()
#endif


using namespace rack;


static void fatalSignalHandler(int sig) {
	// Ignore this signal to avoid recursion.
	signal(sig, NULL);

	std::string stackTrace = system::getStackTrace();
	FATAL("Fatal signal %d. Stack trace:\n%s", sig, stackTrace.c_str());

	// Re-raise signal
	raise(sig);
}

/**
 * Creates the main window if not in headless mode. Also puts the window into 
 * full screen mode if so configured. These multiple steps have been put into 
 * this one function since they are very order dependent. This function uses 
 * settings so they must be read in first.
 */
static void initUI() {
	// Initialize context. This needs to be done before Window created.
    Context* context = new Context();
	contextSet(context);

	// If in headless mode then don't need to create window, so done
    if (settings::headless) return;

    // Initialize UI and create main window
    ui::init();
    window::Window::init();
    context->createWindow();

    // Step window to make splash screen appear
    INFO("Making splash screen visible...");
    getWindow()->step(); 

    // If was in full screen mode previously go right into full screen mode
    if (settings::windowMaximized) {
        INFO("Putting window into full screen mode");
        getWindow()->setFullScreen(true);
    }
}

/** Prints command line usage to stderr */
static void printUsage() {
    std::string msg = R"(
To launch Rack from the command line, cd into Rack\’s folder, and run ./Rack, optionally with the following options.

 * <patch filename>: Loads a patch file.
 * --help: Prints this help message and exits.
 * -l / --liminal: Configures Rack to run as Liminal, a special edition of Rack for installations and live performance.
 * -b / --debug: Enables debug logging to the log file.
 * -r / --trace: Enables trace logging to the log file.
 * -s / --system <Rack system folder>: Sets Rack’s system folder, containing read-only program resources. Defaults to
   - MacOS: <app bundle path>/Contents/Resources
   - Windows: install location, such as C:\\Program Files\\VCV\\Rack
   - Linux: current working directory
 * -u / --user <Rack user folder>: Sets Rack’s user folder, containing settings, plugins, and patches. See Where is the “Rack user folder”? for the default location.
 * -d / --dev: Enables development mode. This sets the system and user folders to the current working directory, uses the terminal (stderr) for logging, and disables Rack’s Library menu to prevent overwriting plugins.
 * -h / --headless: Launches the autosaved patch with no window. Great for generative patches in museum exhibits. Patch can be controlled with MIDI.
 * -a / --safe: Launches Rack with no plugins or autosave patch. Useful for testing.
 * -t / --screenshot <zoom factor>: Captures screenshots of all installed modules and saves each to <Rack user folder>/screenshots/<plugin slug>/<module slug>.png. A zoom factor of 1 generates a screenshot with 380px height.
 * -v / --version: Prints Rack version and exits.
 
See https://vcvrack.com/manual/Installing#Command-line-usage 
)";
    std::fprintf(stderr, "%s", msg.c_str());
}

/**
 * THe main entry point for Rack. Starts up the whole application.
 */
int main(int argc, char* argv[]) {
#if defined ARCH_WIN
	// Windows global mutex to prevent multiple instances
	// Handle will be closed by Windows when the process ends
	HANDLE instanceMutex = CreateMutexW(NULL, true, string::UTF8toUTF16(APP_NAME).c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, "VCV Rack is already running. Multiple Rack instances are not supported.");
		exit(1);
	}
	(void) instanceMutex;

	// Don't display "Assertion failed!" dialog message.
	_set_error_mode(_OUT_TO_STDERR);
#endif

	std::string patchPath;
	bool screenshot = false;
	float screenshotZoom = 1.f;
	const std::string appInfo = APP_NAME + " " + APP_EDITION_NAME + APP_OS_NAME + " " + APP_CPU_NAME;

	// Parse command line arguments
	static const struct option longOptions[] = {
		{"liminal", no_argument, NULL, 'l'},
		{"debug", no_argument, NULL, 'b'},
		{"trace", no_argument, NULL, 'r'},
		{"safe", no_argument, NULL, 'a'},
		{"dev", no_argument, NULL, 'd'},
		{"headless", no_argument, NULL, 'h'},
		{"screenshot", required_argument, NULL, 't'},
		{"system", required_argument, NULL, 's'},
		{"user", required_argument, NULL, 'u'},
		{"version", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 256},
		{NULL, 0, NULL, 0}
	};
	int c;
	opterr = 0;

	while ((c = getopt_long(argc, argv, "lbradht:s:u:vp:", longOptions, NULL)) != -1) {
        switch (c) {
            case 'l':
                rack::ui::Liminal::configAsLiminal();
                break;
            case 'b':
                // Turn on debug logging
                logger::setLogLevel(logger::Level::DEBUG_LEVEL);
                break;
            case 'r':
                // Turn on debug logging
                logger::setLogLevel(logger::Level::TRACE_LEVEL);
                break;
            case 'a':
                settings::safeMode = true;
                break;
            case 'd':
                settings::devMode = true;
                break;
            case 'h':
                settings::headless = true;
                break;
            case 't':
                screenshot = true;
                std::sscanf(optarg, "%f", &screenshotZoom);
                break;
            case 's':
                asset::systemDir = optarg;
                break;
            case 'u':
                asset::userDir = optarg;
                break;
            case 'v':
                std::fprintf(stderr, "%s\n", appInfo.c_str());
                return 0;
                break;
            case 256:  // --help
                std::fprintf(stderr, "%s\n", appInfo.c_str());
                printUsage();
                return 0;
                break;
            // Mac "app translocation" passes a nonsense -psn_... flag, so -p is
            // reserved.
            case 'p':
                break;
            default:
                break;
        }
    }
	if (optind < argc) {
		patchPath = argv[optind];
	}

    // Seed random number generator so actually get random numbers
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// Initialize environment
	system::init();
	system::resetFpuFlags();
	asset::init();
	if (!settings::devMode) {
		logger::logPath = asset::user("log.txt");
	}
	logger::init();
	random::init();

	// Now that logging fully setup log the log level
	logger::logLogLevel();

	// We can now install a signal handler and log the output
	if (!settings::devMode) {
		signal(SIGABRT, fatalSignalHandler);
		signal(SIGFPE, fatalSignalHandler);
		signal(SIGILL, fatalSignalHandler);
		signal(SIGSEGV, fatalSignalHandler);
		signal(SIGTERM, fatalSignalHandler);
	}

	// Log environment
    INFO("Running Rack standalone...");
	INFO("%s", appInfo.c_str());
	INFO("%s", system::getOperatingSystemInfo().c_str());
	std::string argsList;
	for (int i = 0; i < argc; i++) {
		argsList += argv[i];
		argsList += " ";
	}
	INFO("Args: %s", argsList.c_str());
	if (settings::devMode)
		INFO("Development mode");
	INFO("System directory: %s", asset::systemDir.c_str());
	INFO("User directory: %s", asset::userDir.c_str());
#if defined ARCH_MAC
	INFO("Bundle path: %s", asset::bundlePath.c_str());
#endif
	INFO("System time: %s", string::formatTimeISO(system::getUnixTime()).c_str());
    INFO("Physical core count: %d", system::getPhysicalCoreCount());
    INFO("Logical core count: %d", system::getLogicalCoreCount());
    
    // Initialize string language translations
	string::init();

	// Load configuration settings
	settings::init();
	try {
		settings::load();
	} catch (Exception& e) {
		std::string msg = e.what();
		msg += "\n\n";
		msg += string::translate("standalone.resetSettings");
		if (!osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK_CANCEL, msg.c_str())) {
			exit(1);
		}
	}

	// Check existence of the system res/ directory
	std::string resDir = asset::system("res");
	if (!system::isDirectory(resDir)) {
		std::string message = string::f(string::translate("standalone.resDir"), resDir);
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, message.c_str());
		exit(1);
	}
	
	INFO("Initializing network");
	network::init();

	// Initialize main UI window. This should be done before initializing
    // plugins and audio so that splash screen can be displayed ASAP.
    INFO("Initializing UI");
	initUI();

    INFO("Initializing plugins (packages of modules from a manufacturer)");
	plugin::init();

	INFO("Initializing audio");
	audio::init();
	rtaudioInit();
#if defined ARCH_MAC
	if (rtaudioIsMicrophoneBlocked()) {
		std::string msg = string::f(string::translate("standalone.micPermission"), APP_NAME + " " + APP_VERSION_MAJOR + " " + APP_EDITION_NAME);
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, msg.c_str());
	}
#endif
	INFO("Initializing MIDI");
	midi::init();
	rtmidiInit();
	keyboard::init();
	gamepad::init();
	midiloopback::init();

	INFO("Initializing module browser");
	app::browser::browserInit();
    
	INFO("Initializing module library");
	library::init();

	// On Mac, use a hacked-in GLFW addition to get the launched path.
#if defined ARCH_MAC
	// For some reason, launching from the command line sets glfwGetOpenedFilenames(), so make sure we're running the app bundle.
	if (asset::bundlePath != "") {
		const char* const* openedFilenames = glfwGetOpenedFilenames();
		if (openedFilenames && openedFilenames[0]) {
			patchPath = openedFilenames[0];
		}
	}
#endif

    // Initialize patch
	if (logger::wasTruncated() && osdialog_message(OSDIALOG_INFO, OSDIALOG_YES_NO, string::translate("standalone.crashed").c_str())) {
		// Do nothing, which leaves a blank patch
	}
	else {
		getPatch()->launch(patchPath);
	}

    // Run context
	if (settings::headless) {
		printf("Press enter to exit.\n");
		getchar();
	}
	else if (screenshot) {
		INFO("Taking screenshots of all modules at %gx zoom", screenshotZoom);
		getWindow()->screenshotModules(asset::user("screenshots"), screenshotZoom);
	} else {
        // Leave splash screen up for default time
        app::SplashWidget::waitTillSplashShouldCloseAutomatically();

		// Run till user exits
		getWindow()->mainLoop();
	}

	// Time to exit. First, destroy context
	INFO("Deleting context");
	delete APP;
	contextSet(NULL);
	if (!settings::headless) {
		settings::save();
	}

	// Destroy environment
	if (!settings::headless) {
		INFO("Destroying window");
		window::Window::destroy();
		INFO("Destroying UI");
		ui::destroy();
	}
	INFO("Destroying library");
	library::destroy();
	INFO("Destroying keyboard");
	keyboard::destroy();
	INFO("Destroying MIDI");
	midi::destroy();
	INFO("Destroying audio");
	audio::destroy();
	INFO("Destroying plugins");
	plugin::destroy();
	INFO("Destroying network");
	network::destroy();
	settings::destroy();
	INFO("Destroying logger");
	logger::destroy();

	// Restart executable if requested
	if (settings::restart) {
#if defined ARCH_WIN
		CloseHandle(instanceMutex);
#endif
		settings::restart = false;
		return main(argc, argv);
	}
	return 0;
}


#ifdef UNICODE
/** UTF-16 to UTF-8 wrapper for Windows with unicode */
int wmain(int argc, wchar_t* argvU16[]) {
	// Initialize char* array with string-owned buffers
	std::string argvStr[argc];
	const char* argvU8[argc + 1];
	for (int i = 0; i < argc; i++) {
		argvStr[i] = string::UTF16toUTF8(argvU16[i]);
		argvU8[i] = argvStr[i].c_str();
	}
	argvU8[argc] = NULL;
	return main(argc, (char**) argvU8);
}
#endif
