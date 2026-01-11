#include <arch.hpp>

#include <map>
#include <queue>
#include <thread>

#if defined ARCH_MAC
	// For CGAssociateMouseAndMouseCursorPosition
	#include <ApplicationServices/ApplicationServices.h>
#endif

#include <stb_image_write.h>
#include <osdialog.h>

#include <window/Window.hpp>
#include <window/WaylandTouch.hpp>
#include <asset.hpp>
#include <widget/Widget.hpp>
#include <app/Scene.hpp>
#include <keyboard.hpp>
#include <gamepad.hpp>
#include <context.hpp>
#include <patch.hpp>
#include <settings.hpp>
#include <plugin.hpp> // used in Window::screenshot
#include <system.hpp> // used in Window::screenshot

#if defined ARCH_LIN
	// For XkbGetState for directly getting mod keys
	#include <X11/XKBlib.h>
	// For glfwGetX11Display()
	#define GLFW_EXPOSE_NATIVE_WAYLAND
	#include <GLFW/glfw3native.h>
#endif


namespace rack {
namespace window {

// Specifies the minimum window size that can be set by the user
static const math::Vec WINDOW_SIZE_MIN = math::Vec(640, 480);


Font::~Font() {
	// There is no NanoVG deleteFont() function yet, so do nothing
}


void Font::loadFile(const std::string& filename, NVGcontext* vg) {
	this->vg = vg;
	std::string name = system::getStem(filename);
	size_t size;
	// Transfer ownership of font data to font object
	uint8_t* data = system::readFile(filename, &size);
	// Don't use nvgCreateFont because it doesn't properly handle UTF-8 filenames on Windows.
	handle = nvgCreateFontMem(vg, name.c_str(), data, size, 0);
	if (handle < 0) {
		std::free(data);
		throw Exception("Failed to load font %s", filename.c_str());
	}
	INFO("Loaded font %s", filename.c_str());
}


std::shared_ptr<Font> Font::load(const std::string& filename) {
	return getWindow()->loadFont(filename);
}


Image::~Image() {
	// TODO What if handle is invalid?
	if (handle >= 0)
		nvgDeleteImage(vg, handle);
}


void Image::loadFile(const std::string& filename, NVGcontext* vg) {
	this->vg = vg;
	std::vector<uint8_t> data = system::readFile(filename);
	// Don't use nvgCreateImage because it doesn't properly handle UTF-8 filenames on Windows.
	handle = nvgCreateImageMem(vg, NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY, data.data(), data.size());
	if (handle <= 0)
		throw Exception("Failed to load image %s", filename.c_str());
	INFO("Loaded image %s", filename.c_str());
}


std::shared_ptr<Image> Image::load(const std::string& filename) {
	return getWindow()->loadImage(filename);
}

/** Note: since Window is used by the custom modules out there, definition
 * of Internal must remain stable. This is because those modules are compiled against
 * the standard VCVRack version of this file, and so the members in this
 * header must remain exactly the same for binary compatibility.
 */
struct Window::Internal {
	std::string lastWindowTitle_;

	int lastWindowX_ = 0;
	int lastWindowY_ = 0;
	int lastWindowWidth_ = 0;
	int lastWindowHeight_ = 0;

	int frameCount_ = 0;

	bool cursorLocked_ = false;
	math::Vec cursorLockedPos_;
	double ignoreMouseDeltaUntil_ = -INFINITY;
	double monitorRefreshRate_ = 0.0;

    // For making calculations related to frame timing
	double frameStartTime_ = NAN;

    // Desired frame time in seconds based on frame rate limit.
    // If set to zero (or really low value) then frames will be 
    // processed as fast as possible.
    double desiredFrameDuration_ = 1.f / settings::frameRateLimit;

    // For getting sleep time for frame rate just right
    double frameTimingOffset_ = 0;

    // 1/fps. So can display frame rate being achieved
    double lastFrameDuration_ = NAN;

    // 1/fps. So can display potential frame rate
    double actualFrameProcessingDuration_ = NAN;

    // Last mouse position
	math::Vec lastMousePos_;

    // Caches for loaded fonts and images
    std::map<std::string, std::shared_ptr<Font>> fontCache_;
    std::map<std::string, std::shared_ptr<Image>> imageCache_;

    bool fbDirtyOnSubpixelChange_ = true;
	int fbCount_ = 0;

    /** Called at start of frame to record timing info */
    void startOfFrame();

    /** Determines and stores lastFrameDuration_ so that can be displayed in UI */
    void endOfFrame();

    /** Called at start of frame to record timing info. Returns
     * true if should continue stepping through frames.
     */
    bool shouldContinueStepping(GLFWwindow* glfWin);
};

static void windowPosCallback(GLFWwindow* win, int x, int y) {
	if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED))
		return;
	if (glfwGetWindowAttrib(win, GLFW_ICONIFIED))
		return;
	if (glfwGetWindowMonitor(win))
		return;
	settings::windowPos = math::Vec(x, y);
	DEBUG("windowPosCallback %d %d", x, y);
}

static void windowSizeCallback(GLFWwindow* win, int width, int height) {
	if (glfwGetWindowAttrib(win, GLFW_MAXIMIZED))
		return;
	if (glfwGetWindowAttrib(win, GLFW_ICONIFIED))
		return;
	if (glfwGetWindowMonitor(win))
		return;
	settings::windowSize = math::Vec(width, height);
	DEBUG("windowSizeCallback(%d, %d)", width, height);
}


/** Called when the window is maximized or restored since it is enabled using
 * glfwSetWindowMaximizeCallback(). Unfortunately this means that can actually 
 * be called multiple times in quick succession, so we need to be careful
 * about how we handle the state. For example, when restoring from maximized
 * to normal windowed mode, this callback can be called twice: once for getting
 * out of full screen (maximized set to false), and then again for restoring
 * the window to full size as opposed to full screen (maximized set to true).
 * Since the maximized parameter can be true for full screen or for full size
 * mode it can't be relied on to determine if in full screen mode or not.
 */
static void windowMaximizeCallback(GLFWwindow* win, int maximized) {
}


static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
	contextSet((Context*) glfwGetWindowUserPointer(win));
#if defined ARCH_MAC
	// Remap Ctrl-left click to right click on Mac
	if (button == GLFW_MOUSE_BUTTON_LEFT && (mods & RACK_MOD_MASK) == GLFW_MOD_CONTROL) {
		button = GLFW_MOUSE_BUTTON_RIGHT;
		mods &= ~GLFW_MOD_CONTROL;
	}
	// Remap Ctrl-shift-left click to middle click on Mac
	if (button == GLFW_MOUSE_BUTTON_LEFT && (mods & RACK_MOD_MASK) == (GLFW_MOD_CONTROL | GLFW_MOD_SHIFT)) {
		button = GLFW_MOUSE_BUTTON_MIDDLE;
		mods &= ~(GLFW_MOD_CONTROL | GLFW_MOD_SHIFT);
	}
#endif

	getEvent()->handleButton(getWindow()->getLastMousePos(), button, action, mods);
}

// FIXME just for testing
static void cursorPosCallbackTest(GLFWwindow* win, double xpos, double ypos) {
	DEBUG("==> FIXME cursorPosCallbackTest x=%.2f y=%.2f", xpos, ypos);
    auto thing = (Context*) glfwGetWindowUserPointer(win);
}

static void cursorPosCallback(GLFWwindow* win, double xpos, double ypos) {
	contextSet((Context*) glfwGetWindowUserPointer(win));
	Window* window = getWindow();
	int width, height;
	glfwGetWindowSize(win, &width, &height);
	float ratio = window->pixelRatio_ / window->windowRatio_;

	math::Vec mousePos;
	math::Vec mouseDelta;

	if (window->isCursorLocked()) {
		mousePos = (window->getCursorLockedPos() / ratio).round();
		mouseDelta = math::Vec(xpos - width / 2, ypos - height / 2) / ratio;

		// Reset cursor to center of screen
		glfwSetCursorPos(win, width / 2, height / 2);
	}
	else {
		mousePos = (math::Vec(xpos, ypos) / ratio).round();
		mouseDelta = mousePos - window->getLastMousePos();

		window->setLastMousePos(mousePos);
	}

    getEvent()->handleHover(mousePos, mouseDelta);

	if (!window->isCursorLocked()) {
		// Keyboard/mouse MIDI driver
		math::Vec scaledPos(xpos / width, ypos / height);
		keyboard::mouseMove(scaledPos);
	}
}

static void cursorEnterCallback(GLFWwindow* win, int entered) {
	DEBUG("cursorEnterCallback entered=%d", entered);

	contextSet((Context*) glfwGetWindowUserPointer(win));
	if (!entered) {
		getEvent()->handleLeave();
	}
}


static void scrollCallback(GLFWwindow* win, double x, double y) {
	DEBUG("scrollCallback x=%.2f y=%.2f", x, y);

	contextSet((Context*) glfwGetWindowUserPointer(win));
	math::Vec scrollDelta = math::Vec(x, y);
#if defined ARCH_MAC
	scrollDelta = scrollDelta.mult(10.0);
#else
	scrollDelta = scrollDelta.mult(50.0);
#endif

	getEvent()->handleScroll(getWindow()->getLastMousePos(), scrollDelta);
}


static void charCallback(GLFWwindow* win, unsigned int codepoint) {
	contextSet((Context*) glfwGetWindowUserPointer(win));
	if (getEvent()->handleText(getWindow()->getLastMousePos(), codepoint))
		return;
}


static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
	contextSet((Context*) glfwGetWindowUserPointer(win));
	if (getEvent()->handleKey(getWindow()->getLastMousePos(), key, scancode, action, mods))
		return;

	// Keyboard/mouse MIDI driver
	if (action == GLFW_PRESS && (mods & RACK_MOD_MASK) == 0) {
		keyboard::press(key);
	}
	if (action == GLFW_RELEASE) {
		keyboard::release(key);
	}
}


static void dropCallback(GLFWwindow* win, int count, const char** paths) {
	contextSet((Context*) glfwGetWindowUserPointer(win));
	std::vector<std::string> pathsVec;
	for (int i = 0; i < count; i++) {
		pathsVec.push_back(paths[i]);
	}
	getEvent()->handleDrop(getWindow()->getLastMousePos(), pathsVec);
}


static void errorCallback(int error, const char* description) {
	WARN("GLFW error %d: %s", error, description);
}


Window::Window() {
    INFO("Constructing Window...");

	internal_ = new Internal();
	int err;

	// Set window hints
#if defined NANOVG_GL2
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined NANOVG_GL3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

#if defined ARCH_MAC
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

	// Create window
	glfWin_ = glfwCreateWindow(1024, 720, "", NULL, NULL);
	if (!glfWin_) {
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, "Could not open GLFW window. Does your graphics card support OpenGL 2.0 or greater? If so, make sure you have the latest graphics drivers installed.");
		throw Exception("Could not create Window");
	}

	float contentScale;
	glfwGetWindowContentScale(glfWin_, &contentScale, NULL);
	DEBUG("Window content scale: %f", contentScale);

	glfwSetWindowSizeLimits(glfWin_, WINDOW_SIZE_MIN.getX(), WINDOW_SIZE_MIN.getY(), GLFW_DONT_CARE, GLFW_DONT_CARE);
	if (settings::windowSize.getX() > 0 && settings::windowSize.getY() > 0) {
		glfwSetWindowSize(glfWin_, settings::windowSize.getX(), settings::windowSize.getY());
	}
	if (settings::windowPos.getX() > -32000 && settings::windowPos.getY() > -32000) {
		glfwSetWindowPos(glfWin_, settings::windowPos.getX(), settings::windowPos.getY());
	}
	if (settings::windowMaximized) {
		glfwMaximizeWindow(glfWin_);
	}
	glfwShowWindow(glfWin_);

	glfwSetWindowUserPointer(glfWin_, contextGet());
	glfwSetInputMode(glfWin_, GLFW_LOCK_KEY_MODS, 1);

	glfwMakeContextCurrent(glfWin_);
	glfwSwapInterval(0);
	const GLFWvidmode* monitorMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	if (monitorMode->refreshRate > 0) {
		internal_->monitorRefreshRate_ = monitorMode->refreshRate;
	}
	else {
		// Some monitors report 0Hz refresh rate for some reason, so as a workaround, assume 60Hz.
		internal_->monitorRefreshRate_ = 60;
	}

	// Set window callbacks
	glfwSetWindowPosCallback(glfWin_, windowPosCallback);
	glfwSetWindowSizeCallback(glfWin_, windowSizeCallback);
	glfwSetWindowMaximizeCallback(glfWin_, windowMaximizeCallback);
	glfwSetMouseButtonCallback(glfWin_, mouseButtonCallback);
    glfwSetCursorPosCallback(glfWin_, cursorPosCallbackTest); // FIXME 
	// Call this ourselves, but on every frame instead of only when the mouse moves
	// glfwSetCursorPosCallback(win, cursorPosCallback);
	glfwSetCursorEnterCallback(glfWin_, cursorEnterCallback);
	glfwSetScrollCallback(glfWin_, scrollCallback);
	glfwSetCharCallback(glfWin_, charCallback);
	glfwSetKeyCallback(glfWin_, keyCallback);
	glfwSetDropCallback(glfWin_, dropCallback);

	// Set up GLEW
    //FIXME taken out because glewInit() not working with Wayland
	glewExperimental = GL_TRUE;
	err = glewInit();
	if (err != GLEW_OK) {
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, "Could not initialize GLEW. Does your graphics card support OpenGL 2.0 or greater? If so, make sure you have the latest graphics drivers installed.");
		throw Exception("Could not initialize GLEW");
	}

	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	INFO("Renderer: %s %s", vendor, renderer);
	INFO("OpenGL: %s", version);

	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();

	// Set up NanoVG
	int nvgFlags = NVG_ANTIALIAS;
#if defined NANOVG_GL2
	vg_ = nvgCreateGL2(nvgFlags);
	fbVg_ = nvgCreateSharedGL2(vg_, nvgFlags);
#elif defined NANOVG_GL3
	vg = nvgCreateGL3(nvgFlags);
#elif defined NANOVG_GLES2
	vg = nvgCreateGLES2(nvgFlags);
#endif
	if (!vg_) {
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, "Could not initialize NanoVG. Does your graphics card support OpenGL 2.0 or greater? If so, make sure you have the latest graphics drivers installed.");
		throw Exception("Could not initialize NanoVG");
	}

	// Load and set the main font to be used
	uiFont_ = loadFont(asset::system(settings::systemFontFileName));
	if (uiFont_)
		bndSetFont(uiFont_->handle);

	if (getScene()) {
        // Notify all widgets that the Scene context has been created
		widget::Widget::ContextCreateEvent e;
		e.vg = vg_;
		getScene()->onContextCreate(e);
	}

    INFO("Constructed Window");
}


Window::~Window() {
	if (getScene()) {
		widget::Widget::ContextDestroyEvent e;
		e.vg = vg_;
		getScene()->onContextDestroy(e);
	}

	// Fonts and Images in the cache must be deleted before the NanoVG context is deleted
	internal_->fontCache_.clear();
	internal_->imageCache_.clear();

	// nvgDeleteClone(fbVg);

#if defined NANOVG_GL2
	nvgDeleteGL2(vg_);
	nvgDeleteGL2(fbVg_);
#elif defined NANOVG_GL3
	nvgDeleteGL3(vg);
#elif defined NANOVG_GLES2
	nvgDeleteGLES2(vg);
#endif

	glfwDestroyWindow(glfWin_);
	delete internal_;
}


math::Vec Window::getSize() {
	int width, height;
	glfwGetWindowSize(glfWin_, &width, &height);
	return math::Vec(width, height);
}


void Window::setSize(math::Vec size) {
	size = size.max(WINDOW_SIZE_MIN);
	glfwSetWindowSize(glfWin_, size.getX(), size.getY());
}


bool Window::Internal::shouldContinueStepping(GLFWwindow* glfWin) {
    // Record timing info at start of frame. Do this right at beginning of frame
    // so that other calls don't effect the timing.
    startOfFrame();

    // Return whether should continue
    return !glfwWindowShouldClose(glfWin);
}


void Window::Internal::startOfFrame() {
    // Record start time of frame
    frameStartTime_ = system::getTime();
}


void Window::Internal::endOfFrame() {
  // Wait appropiate amount of time to achieve desired frame rate.
  double currentTimeBeforeSleep = system::getTime();
  actualFrameProcessingDuration_ = currentTimeBeforeSleep - frameStartTime_;
  double desiredFrameDoneTime = frameStartTime_ + desiredFrameDuration_;
  double timeToSleep = desiredFrameDoneTime - currentTimeBeforeSleep - frameTimingOffset_;

  if (timeToSleep > 0) {
    system::sleep(timeToSleep);

    // Since had to sleep was able to achieve desired frame time. 
    // So record desired frame time as the achieved frame time.
    lastFrameDuration_ = desiredFrameDuration_;

    // Update frameTimingOffset_ so that can compensate for it. Use the
    // average of the the curret offset and the previous offset so that
    // it doesn't jump around as much.
    double currentFrameTimingOffset = system::getTime() - desiredFrameDoneTime;
    frameTimingOffset_ = (currentFrameTimingOffset + frameTimingOffset_) / 2;
  } else {
    // Took more than allocated time so frame rate needs to be determined
    lastFrameDuration_ = currentTimeBeforeSleep - frameStartTime_;
  }
}

void Window::mainLoop() {
    INFO("Running window main loop...");

    while (internal_->shouldContinueStepping(getGLFWwindow())) {
        // Process the frame and recurse through all child widgets
        step();

        // Log every 180 frames just to show that app is still running
        static logger::LogCounter frameCounter(180);
        if (frameCounter.shouldLog()) {
            DEBUG("Processed frame %d", internal_->frameCount_);
        }

        // Wait till done with allocated frame time
        internal_->endOfFrame();
    }

    INFO("Stopped window main loop");
}

void Window::step() {
    // Keep track of number of frames processed
    ++internal_->frameCount_;

    internal_->fbCount_ = 0;

    // Make event handlers and step() have a clean NanoVG context
    nvgReset(vg_);

    bndSetFont(uiFont_->handle);

    // Poll events
    // Save and restore context because event handler set their own context
    // based on which window they originate from.
    Context* context = contextGet();
    glfwPollEvents();
	contextSet(context);

	// In case glfwPollEvents() sets another OpenGL context
	glfwMakeContextCurrent(glfWin_);

	// Call cursorPosCallback every frame, not just when the mouse moves
	{
		double xpos, ypos;
		glfwGetCursorPos(glfWin_, &xpos, &ypos);
		cursorPosCallback(glfWin_, xpos, ypos);
	}
	gamepad::step();

	// Set window title
	std::string windowTitle = APP_NAME + " " + APP_EDITION_NAME + " " + APP_VERSION;
	if (getPatch()->path != "") {
		windowTitle += " - ";
		if (!getHistory()->isSaved())
			windowTitle += "*";
		windowTitle += system::getFilename(getPatch()->path);
	}
	if (windowTitle != internal_->lastWindowTitle_) {
		glfwSetWindowTitle(glfWin_, windowTitle.c_str());
		internal_->lastWindowTitle_ = windowTitle;
	}

	// Get desired pixel ratio
	float newPixelRatio;
	if (settings::pixelRatio > 0.0) {
		newPixelRatio = settings::pixelRatio;
	}
	else {
		glfwGetWindowContentScale(glfWin_, &newPixelRatio, NULL);
		newPixelRatio = std::floor(newPixelRatio + 0.5);
	}
	if (newPixelRatio != pixelRatio_) {
		pixelRatio_ = newPixelRatio;
		getEvent()->handleDirty();
	}

	// Get framebuffer/window ratio
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(glfWin_, &fbWidth, &fbHeight);
	int winWidth, winHeight;
	glfwGetWindowSize(glfWin_, &winWidth, &winHeight);
	windowRatio_ = (float)fbWidth / winWidth;

	if (getScene()) {
		// Resize scene
		getScene()->setSize(math::Vec(fbWidth, fbHeight).div(pixelRatio_));

		// Step scene
		getScene()->step();

		// Render scene
		bool visible = glfwGetWindowAttrib(glfWin_, GLFW_VISIBLE) && !glfwGetWindowAttrib(glfWin_, GLFW_ICONIFIED);
		if (visible) {
			// Update and render
			nvgBeginFrame(vg_, fbWidth, fbHeight, pixelRatio_);
			nvgScale(vg_, pixelRatio_, pixelRatio_);

			// Draw scene
			widget::Widget::DrawArgs args;
			args.vg = vg_;
			args.clipBox = getScene()->getBox().zeroPos();
			getScene()->draw(args);

			glViewport(0, 0, fbWidth, fbHeight);
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			nvgEndFrame(vg_);
		}
	}

	glfwSwapBuffers(glfWin_);
}

static void flipBitmap(uint8_t* pixels, int width, int height, int depth) {
	for (int y = 0; y < height / 2; y++) {
		int flipY = height - y - 1;
		std::vector<uint8_t> tmp(width * depth);
		std::memcpy(tmp.data(), &pixels[y * width * depth], width * depth);
		std::memcpy(&pixels[y * width * depth], &pixels[flipY * width * depth], width * depth);
		std::memcpy(&pixels[flipY * width * depth], tmp.data(), width * depth);
	}
}

void Window::screenshot(const std::string& screenshotPath) {
    // Get window framebuffer size
    int width, height;
    glfwGetFramebufferSize(getWindow()->glfWin_, &width, &height);

    // Allocate pixel color buffer
    uint8_t* pixels = new uint8_t[height * width * 4];

    // glReadPixels defaults to GL_BACK, but the back-buffer is unstable, so use
    // the front buffer (what the user sees)
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Write pixels to PNG
    flipBitmap(pixels, width, height, 4);
    stbi_write_png(screenshotPath.c_str(), width, height, 4, pixels, width * 4);

    delete[] pixels;
}

void Window::screenshotModules(const std::string& screenshotsDir, float zoom) {
	// Disable preferDarkPanels
	bool preferDarkPanels = settings::preferDarkPanels;
	settings::preferDarkPanels = false;
	DEFER({settings::preferDarkPanels = preferDarkPanels;});

	// Iterate plugins and create directories
	system::createDirectories(screenshotsDir);
	for (plugin::Plugin* p : plugin::plugins) {
		std::string dir = system::join(screenshotsDir, p->slug);
		system::createDirectory(dir);
		for (plugin::Model* model : p->models) {
			std::string filename = system::join(dir, model->slug + ".png");

			// Skip model if screenshot already exists
			if (system::isFile(filename))
				continue;

			INFO("Screenshotting %s %s to %s", p->slug.c_str(), model->slug.c_str(), filename.c_str());

			// Create widgets
			widget::FramebufferWidget* fbw = new widget::FramebufferWidget;
			fbw->setOversample(2);

			struct ModuleWidgetContainer : widget::Widget {
				void draw(const DrawArgs& args) override {
					Widget::draw(args);
					Widget::drawLayer(args, 1);
				}
			};
			ModuleWidgetContainer* mwc = new ModuleWidgetContainer;
			fbw->addChild(mwc);

			app::ModuleWidget* mw = model->createModuleWidget(NULL);
            mwc->setSize(mw->getSize());
            fbw->setSize(mw->getSize());
			mwc->addChild(mw);

			// Step to allow the ModuleWidget state to set its default appearance.
			fbw->step();

			// Draw to framebuffer
			fbw->render(math::Vec(zoom, zoom));

			// Read pixels
			nvgluBindFramebuffer(fbw->getFramebuffer());
			int width, height;
			nvgImageSize(vg_, fbw->getImageHandle(), &width, &height);
			uint8_t* pixels = new uint8_t[height * width * 4];
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

			// Write pixels to PNG
			flipBitmap(pixels, width, height, 4);
			stbi_write_png(filename.c_str(), width, height, 4, pixels, width * 4);

			// Cleanup
			delete[] pixels;
			nvgluBindFramebuffer(NULL);
			delete fbw;
		}
	}
}


void Window::close() {
	glfwSetWindowShouldClose(glfWin_, GLFW_TRUE);
}

void Window::cursorLock() {
	if (!settings::allowCursorLock)
		return;
	if (isCursorLocked())
		return;

	// GLFW_CURSOR_DISABLED is buggy.
	// https://github.com/glfw/glfw/issues/2523
	// So instead, hide the cursor, move cursor to center of window, and reset mouse position every frame in cursorPosCallback().
    double xpos, ypos;
	glfwGetCursorPos(glfWin_, &xpos, &ypos);
    internal_->cursorLockedPos_ = math::Vec(xpos, ypos); 
	internal_->cursorLocked_ = true;
	glfwSetInputMode(glfWin_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	int width, height;
	glfwGetWindowSize(glfWin_, &width, &height);
	glfwSetCursorPos(glfWin_, width / 2, height / 2);
}

void Window::cursorUnlock() {
	if (!settings::allowCursorLock)
		return;
	if (!internal_->cursorLocked_)
		return;

	// Restore cursor position when locked
	glfwSetCursorPos(glfWin_, getCursorLockedPos().getX(), getCursorLockedPos().getY());
	internal_->cursorLocked_ = false;
	glfwSetInputMode(glfWin_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


bool Window::isCursorLocked() {
	return internal_->cursorLocked_;
}

int Window::getMods() {
    int mods = 0;
#if defined ARCH_LIN
    // On Linux X11, get mods directly from X11 display, to support X11 key
    // remapping.
    // FIXME
    INFO("About to call glfwGetWaylandDisplay()");
    wl_display* wlDisplay = glfwGetWaylandDisplay();
    INFO("Finished it");
    // Display* display =  wlDisplay->display;
    // //Display* display =  glfwGetX11Display();
    // XkbStateRec state;
    // XkbGetState(display, XkbUseCoreKbd, &state);

    // // Derived from GLFW's translateState() from x11_window.c
    // if (state.mods & ShiftMask) mods |= GLFW_MOD_SHIFT;
    // if (state.mods & ControlMask) mods |= GLFW_MOD_CONTROL;
    // if (state.mods & Mod1Mask) mods |= GLFW_MOD_ALT;
    // if (state.mods & Mod4Mask) mods |= GLFW_MOD_SUPER;
    // if (state.mods & LockMask) mods |= GLFW_MOD_CAPS_LOCK;
    // if (state.mods & Mod2Mask) mods |= GLFW_MOD_NUM_LOCK;
#else
    // Use GLFW key codes on other OS's
    if (glfwGetKey(glfWin_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(glfWin_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        mods |= GLFW_MOD_SHIFT;
    if (glfwGetKey(glfWin_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(glfWin_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        mods |= GLFW_MOD_CONTROL;
    if (glfwGetKey(glfWin_, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(glfWin_, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
        mods |= GLFW_MOD_ALT;
    if (glfwGetKey(glfWin_, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(glfWin_, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
        mods |= GLFW_MOD_SUPER;
#endif
    return mods;
}

void Window::setFullScreen(bool fullScreen) {
    // Remember full screen state
    settings::windowMaximized = fullScreen;

    if (!fullScreen) {
        // Put window into non-full screen mode
        INFO("Taking main window out of full screen mode");
        glfwSetWindowMonitor(glfWin_, NULL, internal_->lastWindowX_, internal_->lastWindowY_,
                             internal_->lastWindowWidth_, internal_->lastWindowHeight_, GLFW_DONT_CARE);
    } else {
        // Put window into full screen mode
        INFO("Putting main window into full screen mode");
        glfwGetWindowPos(glfWin_, &internal_->lastWindowX_, &internal_->lastWindowY_);
        glfwGetWindowSize(glfWin_, &internal_->lastWindowWidth_, &internal_->lastWindowHeight_);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(glfWin_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

bool Window::isFullScreen() {
	// Return whether main window is in full screen mode
	GLFWmonitor* monitor = glfwGetWindowMonitor(glfWin_);
	return monitor != NULL;
}

math::Vec Window::getLastMousePos() {
    return internal_->lastMousePos_;
}

math::Vec Window::getCursorLockedPos() {
    return internal_->cursorLockedPos_;
}

void Window::setLastMousePos(const math::Vec& pos) {
    internal_->lastMousePos_ = pos;
}

double Window::getMonitorRefreshRate() {
	return internal_->monitorRefreshRate_;
}

void Window::setFrameRate(double frameRate) const {
    if (frameRate <= 0.0 || std::isnan(frameRate)) {
        // desired frame rate not proper so just run as fast as possible
        internal_->desiredFrameDuration_ = 0.0;
    } else {
        // Set desired frame duration based on frame rate
        internal_->desiredFrameDuration_ = 1.f / frameRate;
    }
}

double Window::getFrameStartTime() const {
	return internal_->frameStartTime_;
}

double Window::getIgnoreMouseDeltaUntil() const {
    return internal_->ignoreMouseDeltaUntil_;
}

double Window::getLastFrameDuration() const {
	return internal_->lastFrameDuration_;
}

double Window::getFrameDurationRemaining() const {
    return internal_->desiredFrameDuration_ -
           (system::getTime() - internal_->frameStartTime_);
}

double Window::getLastFrameRate() const {
    if (internal_->lastFrameDuration_ == 0.0 ||
        std::isnan(internal_->lastFrameDuration_)) {
        return 0.0;
    } else {
        return 1.0 / internal_->lastFrameDuration_;
    }
}

double Window::getPotentialFrameRate() const {
    return 1.0 / internal_->actualFrameProcessingDuration_;
}

std::shared_ptr<Font> Window::loadFont(const std::string& filename) {
	// If font is already cached, no need to add fallback fonts again.
	const auto& it = internal_->fontCache_.find(filename);
	if (it != internal_->fontCache_.end())
		return it->second;

    // This redundantly searches the font cache, but it's not a performance
    // issue because it only happens when font is first loaded.
    std::shared_ptr<Font> font = loadFontWithoutFallbacks(filename);
    if (!font)
		return NULL;

    // Load fallback fonts for CJK and emoji characters
	std::shared_ptr<Font> jpFont = loadFontWithoutFallbacks(asset::system("res/fonts/NotoSansJP-Medium.otf"));
	if (jpFont)
		nvgAddFallbackFontId(vg_, font->handle, jpFont->handle);
	std::shared_ptr<Font> scFont = loadFontWithoutFallbacks(asset::system("res/fonts/NotoSansSC-Medium.otf"));
	if (scFont)
		nvgAddFallbackFontId(vg_, font->handle, scFont->handle);
	std::shared_ptr<Font> emojiFont = loadFontWithoutFallbacks(asset::system("res/fonts/NotoEmoji-Medium.ttf"));
	if (emojiFont)
		nvgAddFallbackFontId(vg_, font->handle, emojiFont->handle);

	return font;
}


std::shared_ptr<Font> Window::loadFontWithoutFallbacks(const std::string& filename) {
	// Return cached font, even if null
	const auto& it = internal_->fontCache_.find(filename);
	if (it != internal_->fontCache_.end())
		return it->second;

	// Load font
	std::shared_ptr<Font> font = std::make_shared<Font>();
	try {
		font->loadFile(filename, vg_);
	}
	catch (Exception& e) {
		WARN("%s", e.what());
		font = NULL;
	}
	internal_->fontCache_[filename] = font;
	return font;
}


void Window::overrideFontFace(const std::string& filename) {
    std::shared_ptr<Font> font = loadFontWithoutFallbacks(filename);
    if (font)
        bndSetFont(font->handle);
}

void Window::resetFontFace() {
    bndSetFont(getWindow()->uiFont_->handle);
}

std::shared_ptr<Image> Window::loadImage(const std::string& filename) {
	const auto& it = internal_->imageCache_.find(filename);
	if (it != internal_->imageCache_.end())
		return it->second;

	// Load image
	std::shared_ptr<Image> image;
	try {
		image = std::make_shared<Image>();
		image->loadFile(filename, vg_);
	}
	catch (Exception& e) {
		WARN("%s", e.what());
		image = NULL;
	}
	internal_->imageCache_[filename] = image;
	return image;
}


bool& Window::fbDirtyOnSubpixelChange() {
	return internal_->fbDirtyOnSubpixelChange_;
}


int& Window::fbCount() {
	return internal_->fbCount_;
}


void Window::init() {
    INFO("Initializing Window system...");

	// Set up GLFW
#if defined ARCH_MAC
	glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_TRUE);
	glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_FALSE);
#endif

	glfwSetErrorCallback(errorCallback);
	int err = glfwInit();
	if (err != GLFW_TRUE) {
		osdialog_message(OSDIALOG_ERROR, OSDIALOG_OK, "Could not initialize GLFW.");
		throw Exception("Could not initialize GLFW");
	}

    // Initialize touch screen support if available
    window::WaylandTouch::init();

    INFO("Done initializing Window system");
}


void Window::destroy() {
	glfwTerminate();
}


} // namespace window
} // namespace rack
