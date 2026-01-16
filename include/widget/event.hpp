#pragma once
#include <vector>
#include <set>

#include <common.hpp>
#include <math.hpp>



/** Remaps Ctrl to Cmd on Mac
Use this instead of GLFW_MOD_CONTROL, since Cmd should be used on Mac in place of Ctrl on Linux/Windows.
*/
#if defined ARCH_MAC
	#define RACK_MOD_CTRL GLFW_MOD_SUPER
	#define RACK_MOD_CTRL_NAME "⌘"
#else
	#define RACK_MOD_CTRL GLFW_MOD_CONTROL
	#define RACK_MOD_CTRL_NAME "Ctrl"
#endif

#define RACK_MOD_SHIFT GLFW_MOD_SHIFT
#define RACK_MOD_SHIFT_NAME "Shift"

#define RACK_MOD_ALT GLFW_MOD_ALT
#define RACK_MOD_ALT_NAME "Alt"

/** Filters actual mod keys from the mod flags.
Use this if you don't care about GLFW_MOD_CAPS_LOCK and GLFW_MOD_NUM_LOCK.
Example usage:
	if ((e.mod & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) ...
*/
#define RACK_MOD_MASK (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER)

/** A key action state representing the the key is (still) being held.
*/
#define RACK_HELD 3


namespace rack {
namespace widget {

// Forward declaration
class Widget;


/** Returns the name of a GLFW key macro.
Printable keys return the key string such as "Q", "=", "\t", etc.
Letters are capitalized.
Does not remap keys based on keyboard layout, so GLFW_KEY_Q always returns "Q".
GLFW_KEY_SPACE returns "Space" translated to the current language.
Non-printable characters return the name of the key in the current language.
Key 0 returns "".
*/
std::string getKeyName(int key);
/** Returns the name of a key command/chord/combo.
For example, getKeyCommandName(GLFW_KEY_Q, GLFW_MOD_CONTROL) == "Ctrl+Q" translated to the current language.
*/
std::string getKeyCommandName(int key, int mods = 0);


/** A per-event state shared and writable by all widgets that recursively handle an event. */
struct EventContext {
	/** Whether the event should continue recursing to children Widgets. */
	bool propagating = true;
	/** Whether the event has been consumed by an event handler and no more handlers should consume the event. */
	bool consumed = false;
	/** The widget that responded to the event. */
	Widget* target = NULL;
};


/** Base class for all events. */
struct BaseEvent {
	EventContext* context = NULL;

	/** Prevents the event from being handled by more Widgets.
	*/
	void stopPropagating() const {
		if (!context)
			return;
		context->propagating = false;
	}
	bool isPropagating() const {
		if (!context)
			return true;
		return context->propagating;
	}
	/** Tells the event handler that a particular Widget consumed the event.
	You usually want to stop propagation as well, so call consume() instead.
	*/
	void setTarget(Widget* w) const {
		if (!context)
			return;
		context->target = w;
	}
	Widget* getTarget() const {
		if (!context)
			return NULL;
		return context->target;
	}
	/** Sets the target Widget and stops propagating.
	A NULL Widget may be passed to consume but not set a target.
	*/
	void consume(Widget* w) const {
		if (!context)
			return;
		context->propagating = false;
		context->consumed = true;
		context->target = w;
	}
	void unconsume() const {
		if (!context)
			return;
		context->consumed = false;
	}
	bool isConsumed() const {
		if (!context)
			return false;
		return context->consumed;
	}
};

class EventState {
   public:
    EventState() {}

   private:
    Widget* rootWidget_ = nullptr;
    /* State widgets
     * Don't set these directly unless you know what you're doing. Use the
     * set*() methods instead.
     */
    // Currently hovered widget
    Widget* hoveredWidget_ = nullptr;

    // Currently dragged widget
    Widget* draggedWidget_ = nullptr;

    // Which mouse button was used to initiate the drag.
    // GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, etc.
    int dragButton_ = 0;

    // Which widget is being drag-hovered
    Widget* dragHoveredWidget_ = nullptr;

    // Currently selected widget
    Widget* selectedWidget_ = nullptr;

    // For double-clicking
    double lastClickTime_ = -INFINITY;

    // For double-clicking
    Widget* lastClickedWidget_ = nullptr;

    // Any keyboard keys held down when event initiated
    std::set<int> heldKeys_;

   public:
    /** For setting root widget (to the Scene widget) */
    void setRootWidget(Widget* w) { rootWidget_ = w; }

    /** Returns root widget, which is typically the Scene */
    Widget* getRootWidget() { return rootWidget_; }

    /** Returns the widget currently being hovered, or nullptr if none */
    Widget* getHoveredWidget() { return hoveredWidget_; }

    /** Returns the widget currently being dragged, or nullptr if none */
    Widget* getDraggedWidget() { return draggedWidget_; }

    /** Returns the widget currently being drag-hovered, or nullptr if none */
    Widget* getDragHoveredWidget() { return dragHoveredWidget_; }

    /** Returns the widget currently being selected, or nullptr if none */
    Widget* getSelectedWidget() { return selectedWidget_; }

    /** Returns the mouse button used to initiate the drag */
    int getDragButton() { return dragButton_; }

    /** Keeps track of which is the widget being hovered. If first time called
     * for this event then initiates an EnterEvent on the widget. Initiates a
     * LeaveEvent on the previously hovered widget.
     */
    void setHoveredWidget(Widget* w);

    /** Keeps track of which is the widget being dragged. If first time
     * called for this event then initiates a DragStartEvent on the widget.
     * Initiates a DragEndEvent on the previously dragged widget.
     * @param w The widget being dragged.
     * @param button The mouse button used to initiate the drag. Either
     * GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, etc.
     */
    void setDraggedWidget(Widget* w, int button);
    
    /** Keeps track of which is the widget being drag-hovered. If first time
     * called for this event then initiates a DragEnterEvent on the widget.
     * Initiates a DragLeaveEvent on the previously drag-hovered widget.
     */
    void setDragHoveredWidget(Widget* w);

    /** Keeps track of which is the widget being selected. If first time called
     * for this event then initiates a SelectEvent on the widget. Initiates a
     * DeselectEvent on the previously selected widget.
     */
    void setSelectedWidget(Widget* w);

    /** DEPRECATED METHODS - use the set*Widget() methods instead */
    DEPRECATED void setHovered(Widget* w) { setHoveredWidget(w); }

    /** DEPRECATED METHODS - use the set*Widget() methods instead */
    DEPRECATED void setDragged(Widget* w, int button) {
        setDraggedWidget(w, button);
    }

    /** DEPRECATED METHODS - use the set*Widget() methods instead */
    DEPRECATED void setDragHovered(Widget* w) { setDragHoveredWidget(w); }

    /** DEPRECATED METHODS - use the set*Widget() methods instead */
    DEPRECATED void setSelected(Widget* w) { setSelectedWidget(w); }

    /** Prepares event state a widget for deletion */
    void finalizeWidget(Widget* w);

    /** A callback to be called when mouse button pressed ore released. Calls
     * onButton() on widget that was clicked on. Sets draggedWidget on press,
     * and clears it on release. Handles double-click detection.
     *
     * @param pos Position of mouse in Scene coordinates.
     * @param button which mouse button clicked on, e.g.
     * GLFW_MOUSE_BUTTON_LEFT
     * @param action GLFW_PRESS or GLFW_RELEASE
     * @param mods Bitwise OR of modifier keys, e.g. RACK_MOD_CTRL
     * @return true if event was consumed by a widget.
     */
    bool handleButton(math::Vec pos, int button, int action, int mods);

    void handleButtonForDrag(widget::Widget* clickedWidget, math::Vec pos,
                             int button, int action, int mods);

    bool handleHover(math::Vec pos, math::Vec mouseDelta);

    /** A callback to be called when the mouse leaves the main window.
     * Always returns true.
     */
    bool handleLeave();

    bool handleScroll(math::Vec pos, math::Vec scrollDelta);

    /** A callback to be called when text input is received. Calls onText()
     * on the hovered widget.
     *
     * @param pos Position of mouse in Scene coordinates.
     * @param codepoint Unicode code point of the character.
     * @return true if event was consumed by a widget.
     */
    bool handleText(math::Vec pos, uint32_t codepoint);

    bool handleKey(math::Vec pos, int key, int scancode, int action, int mods);
    bool handleDrop(math::Vec pos, const std::vector<std::string>& paths);
    bool handleDirty();
};

} // namespace widget
} // namespace rack
