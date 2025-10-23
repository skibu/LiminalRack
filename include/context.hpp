#pragma once
#include <app/RackWidget.hpp>
#include <app/Scene.hpp>
#include <engine/Engine.hpp>
#include <widget/event.hpp>
#include <window/Window.hpp>
#include <common.hpp>
#include <history.hpp>
#include <midiloopback.hpp>
#include <patch.hpp>


namespace rack {

// Forward declaration of midiloopback::Context
namespace midiloopback {
class Context;
}

/** Rack instance state. Contains all the classes that manage the Rack.
 * Note: since Context is used by the custom modules out there, this
 * header must remain stable. This is because those modules are compiled against
 * the standard VCVRack version of this header, and so the members in this
 * header must remain exactly the same for binary compatibility.
 */
class Context {
   public:
    Context();

    ~Context();

    /** Creates the main window. Only to be called if not in headless mode. */
    void createWindow();

    // Convenience method to get the RackWidget
    app::RackWidget* getRack() {
        return scene_->getRack();
    }

    // Convenience method to get the RackScrollWidget
    app::RackScrollWidget* getRackScroll() {
        return scene_->getRackScroll();
    }

    // Convenience method to get the Scene
    app::Scene* getScene() {
        return scene_;
    }

    void setScene(app::Scene* scene) {
        this->scene_ = scene;
    }

    widget::EventState* getEvent() {
        return event_;
    }

    engine::Engine* getEngine() {
        return engine_;
    }

    window::Window* getWindow() {
        return window;
    }

    history::State* getHistory() {
        return history_;
    }

    patch::Manager* getPatch() {
        return patch_;
    }

    midiloopback::Context* getMidiLoopbackContext() {
        return midiLoopbackContext_;
    }

    /* Note: since Context is used by the custom modules out there, this
     * header must remain stable. This is because those modules are compiled
     * against the standard VCVRack version of this header, and so the members
     * in this header must remain exactly the same for binary compatibility. */
   private:
    widget::EventState* event_ = nullptr;
    app::Scene* scene_ = nullptr;
    engine::Engine* engine_ = nullptr;
   public: 
    // custom modules might be accewssing window directly, unfortunately.
    // Therefore it must be public. And the name cannot be changed to append "_"
    window::Window* window = nullptr;
   private:
    history::State* history_ = nullptr;
    patch::Manager* patch_ = nullptr;
    midiloopback::Context* midiLoopbackContext_ = nullptr;
};

/** Global function that returns the global Context pointer */
Context* contextGet();

/** Global function that sets the context for this thread.
 * You must set the context when preparing each thread if the code uses the APP
 * macro in that thread. */
void contextSet(Context* context);

/** Global function that returns the global EventState pointer */
widget::EventState* getEvent();

/** Global function that returns the Scene pointer. Returns null if no context
 * is set. */
app::Scene* getScene();

/** Global function that returns the Window pointer. Returns null if no context
 * is set. */
window::Window* getWindow();

/** Global function that returns the RackWidget pointer. Returns null if no
 * context is set. */
app::RackWidget* getRack();

/** Global function that returns the Engine pointer. Returns null if no context
 * is set. */
engine::Engine* getEngine();

/** Global function that returns the History State pointer. Returns null if no
 * context is set. */
history::State* getHistory();

/** Global function that returns the Patch Manager pointer. Returns null if no context
 * is set. */
patch::Manager* getPatch();

/** Global function that returns the MidiLoopback Context pointer. Returns null if no context
 * is set. */
midiloopback::Context* getMidiLoopbackContext();

/** Deprecated. Use contextGet() or the APP macro to get the current Context. */
DEPRECATED inline Context* appGet() {
    return contextGet();
}

/** Accesses the global Context pointer. Just an alias for contextGet(). 
 * DEPRECATED: Use contextGet() or other methods instead. But need to leave this
 * here for binary compatibility with existing modules.
 */
#define APP rack::contextGet()

}  // namespace rack
