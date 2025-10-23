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
    window = NULL;

    INFO("Creating patch manager");
	patch_ = new patch::Manager;

    INFO("Creating scene");
	scene_ = new app::Scene();

	INFO("Creating event state");
	event_ = new widget::EventState;
    event_->rootWidget = getScene();

	INFO("Creating history state");
	history_ = new history::State;

    INFO("Creating engine");
	engine_ = new engine::Engine;
	engine_->startFallbackThread();

	INFO("Creating MIDI loopback");
	midiLoopbackContext_ = new midiloopback::Context;    
}

Context::~Context() {
	// Deleting NULL is safe in C++.

	// Set pointers to NULL so other objects will segfault when attempting to access them

	INFO("Deleting window");
	delete window;
	window = NULL;

	INFO("Deleting patch manager");
	delete patch_;
	patch_ = NULL;

	INFO("Deleting scene");
	delete scene_;
	scene_ = NULL;

	INFO("Deleting event state");
	delete event_;
	event_ = NULL;

	INFO("Deleting history state");
	delete history_;
	history_ = NULL;

	INFO("Deleting engine");
	delete engine_;
	engine_ = NULL;

	INFO("Deleting MIDI loopback");
	delete midiLoopbackContext_;
	midiLoopbackContext_ = NULL;
}

void Context::createWindow() {
    INFO("Creating window");
    window = new window::Window;
}

// Global context pointer for the current thread
static thread_local Context* threadContext = NULL;

Context* contextGet() {
	return threadContext;
}

widget::EventState* getEvent() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getEvent();
    }
    return NULL;
}

app::Scene* getScene() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getScene();
    }
    return NULL;
}

window::Window* getWindow() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getWindow();
    }
    return NULL;
}

app::RackWidget* getRack() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getRack();
    }
    return NULL;
}

engine::Engine* getEngine() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getEngine();
    }
    return NULL;
}

history::State* getHistory() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getHistory();
    }
    return NULL;
}

patch::Manager* getPatch() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getPatch();
    }
    return NULL;
}

midiloopback::Context* getMidiLoopbackContext() {
    Context* ctx = contextGet();
    if (ctx) {
        return ctx->getMidiLoopbackContext();
    }
    return NULL;
}

// Apple's clang incorrectly compiles this function when -O2 or higher is enabled.
#ifdef ARCH_MAC
__attribute__((optnone))
#endif
void contextSet(Context* context) {
	threadContext = context;
}


} // namespace rack
