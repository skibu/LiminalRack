#include <context.hpp>
#include <window/Window.hpp>
#include <patch.hpp>
#include <engine/Engine.hpp>
#include <app/Scene.hpp>
#include <history.hpp>
#include <midiloopback.hpp>


namespace rack {

Context::Context() {
    // window only gets created if not in headless mode
    window = nullptr;

    INFO("Creating Patch Manager");
	patch_ = new patch::Manager();

    INFO("Creating Scene");
	scene_ = new app::Scene();

	INFO("Creating Event State");
	event_ = new widget::EventState();
    event_->setRootWidget(getScene());

	INFO("Creating History State");
	history_ = new history::State();

    INFO("Creating Engine");
	engine_ = new engine::Engine;
	engine_->startFallbackThread();

	INFO("Creating MIDI loopback");
	midiLoopbackContext_ = new midiloopback::Context;    
}

Context::~Context() {
	// Deleting nullptr is safe in C++.

	// Set pointers to nullptr so other objects will segfault when attempting to access them

	INFO("Deleting window");
	delete window;
	window = nullptr;

	INFO("Deleting patch manager");
	delete patch_;
	patch_ = nullptr;

	INFO("Deleting scene");
	delete scene_;
	scene_ = nullptr;

	INFO("Deleting event state");
	delete event_;
	event_ = nullptr;

	INFO("Deleting history state");
	delete history_;
	history_ = nullptr;

	INFO("Deleting engine");
	delete engine_;
	engine_ = nullptr;

	INFO("Deleting MIDI loopback");
	delete midiLoopbackContext_;
	midiLoopbackContext_ = nullptr;
}

void Context::createWindow() {
    window = new window::Window();
}

// Global context pointer for the current thread
static thread_local Context* threadContext = nullptr;

Context* contextGet() {
	return threadContext;
}

widget::EventState* getEvent() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getEvent();
    }
    
    ERROR("Called getEvent() before context was created.");

    return nullptr;
}

app::Scene* getScene() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getScene();
    }

    ERROR("Called getScene() before context was created.");

    return nullptr;
}

window::Window* getWindow() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getWindow();
    }

    ERROR("Called getWindow() before context was created.");

    return nullptr;
}

app::RackWidget* getRack() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getRack();
    }

    ERROR("Called getRack() before context was created.");

    return nullptr;
}

engine::Engine* getEngine() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getEngine();
    }

    ERROR("Called getEngine() before context was created.");

    return nullptr;
}

history::State* getHistory() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getHistory();
    }

    ERROR("Called getHistory() before context was created.");

    return nullptr;
}

patch::Manager* getPatch() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getPatch();
    }

    ERROR("Called getPatch() before context was created.");

    return nullptr;
}

midiloopback::Context* getMidiLoopbackContext() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getMidiLoopbackContext();
    }

    ERROR("Called getMidiLoopbackContext() before context was created.");

    return nullptr;
}

// Apple's clang incorrectly compiles this function when -O2 or higher is enabled.
#ifdef ARCH_MAC
__attribute__((optnone))
#endif
void contextSet(Context* context) {
	threadContext = context;
}


} // namespace rack
