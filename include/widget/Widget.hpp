#pragma once
#include <list>
#include <map>

#include <common.hpp>
#include <math.hpp>
#include <window/Window.hpp>
#include <color.hpp>
#include <widget/event.hpp>
#include <weakptr.hpp>


namespace rack {
/** Base UI widget types */
namespace widget {


/** A node in the 2D [scene graph](https://en.wikipedia.org/wiki/Scene_graph).
 * The bounding box of a Widget is a rectangle specified by `box` relative to
 * their parent. The appearance is defined by overriding `draw()`, and the
 * behavior is defined by overriding `step()` and `on*()` event handlers.
 */
class Widget : public WeakBase {
   public:
    /** Constructor. Stores name of widget. */
    Widget(const std::string& name = std::string());

    /** Destructor. Deletes all child widgets. You should only delete orphaned
     * widgets*/
    virtual ~Widget();

    /** Returns the name of the widget. Returns the class name if the name for
     * the widget was not configured. */
    std::string getName();

    /** Returns the bounding box of the widget in its parent's coordinate
     * system. Need separate const version of function since plugins compiled 
	 * to the legacy Rack SDK. */
    math::Rect getBox() {
        return box_;
    }

	/** Returns the bounding box of the widget in its parent's coordinate
	 * system. Is properly a const function. */
	math::Rect getBox() const {
        return box_;
    }

    /** Calls setPos() and then setSize(). */
	void setBox(math::Rect box);

    /** Returns the position of the widget. */
    math::Vec getPos() const {
        return box_.getPos();
    }

    /** Alias for getPos(). getPosition() might be used by modules. */
    math::Vec getPosition() const {
        return getPos();
    }

	/** Sets position and triggers RepositionEvent if position changed. */
	void setPos(const math::Vec& pos);

    /** Alias for setPos(). setPosition() might be used by modules. */
    void setPosition(const math::Vec& pos) {
        setPos(pos);
    }

    /** Sets the X position of the widget. */
    void setX(float x) {
        math::Vec pos = getPos();
        pos.setX(x);
        setPos(pos);
    }   

    /** Returns the X position of the widget. */
    float getX() const {
        return getPos().getX();
    }

    /** Sets the Y position of the widget. */
    void setY(float y) {
        math::Vec pos = getPos();
        pos.setY(y);
        setPos(pos);
    }

    /** Returns the Y position of the widget. */
    float getY() const {
        return getPos().getY();
    }

    /** Gets the size of the widget. */
    math::Vec getSize() const {
        return box_.getSize();
    }

    /** Sets size of the widget and triggers ResizeEvent if size changed. */
	void setSize(math::Vec size);

    /** Sets size of the widget and triggers ResizeEvent if size changed. */
    void setSize(float width, float height) {
        setSize(math::Vec(width, height));
    }

    /** Returns the width of the widget. */
    float getWidth() const {
        return getSize().getWidth();
    }

    /** Sets the width of the widget. */
    void setWidth(float width) {
        box_.setWidth(width);
    }

    /** Returns the height of the widget. */
    float getHeight() const {
        return getSize().getHeight();
    }

    /** Sets the height of the widget. */
    void setHeight(float height) {
        box_.setHeight(height);
    }

    /** Returns the parent widget of this widget.
	 * Cannot be const since plugins compiled to the legacy Rack SDK.
	 */

	widget::Widget* getParent() {
        return parent_;
    }

	/** Returns the parent widget of this widget. Is properly a const 
	 * function. 
	 * */
	widget::Widget* getParent() const {
        return parent_;
    }

    /** Returns the list of child widgets */
    std::list<Widget*> getChildren() const {
        return children_;
    }   

    /** Returns whether the widget is visible. 
	 * Cannot be const since plugins compiled to the legacy Rack SDK.
	*/
	bool isVisible() {
        return visible_;
    }

	/** Sets `visible` and triggers ShowEvent or HideEvent if changed. */
	void setVisible(bool visible);

	/** Makes Widget visible and triggers ShowEvent if changed. */
	void show() {
		setVisible(true);
	}

	/** Makes Widget not visible and triggers HideEvent if changed. */
	void hide() {
		setVisible(false);
	}

    /** Sets visibility to false without triggering an event. This
     * can be in constructor of deired Widget subclasses to have the
     * Widget initially hidden.
     */
    void hideInitially() {
        // The widget should be initially hidden
        visible_ = false;
    }

	/** Requests this Widget's parent to delete it in the next step(). */
	void requestDelete();

    /** Returns the smallest rectangle containing this widget's children
     * (visible and invisible) in its local coordinates. Returns `Rect(Vec(inf,
     * inf), Vec(-inf, -inf))` if there are no children.
    */
    virtual math::Rect getChildrenBoundingBox();
	virtual math::Rect getVisibleChildrenBoundingBox();

    /** Returns whether `ancestor` is a parent or distant parent of this
     * widget.
     */
    bool isDescendantOf(Widget* ancestor);

	/** Returns whether `ancestor` is a parent or distant parent of this
	 * widget. Const version.
	 */
	bool isDescendantOf(Widget* ancestor) const {
		return const_cast<Widget*>(this)->isDescendantOf(ancestor);
	}

	/**  Returns `v` (given in this coordinates) transformed into the
	 * coordinate system of `ancestor`. Note that it is critical to 
	 * call this method via the class that v is relative to. There is no scaling. It simply sums
	 * the positions up the parent chain. If `ancestor` is NULL, transforms
	 * `v` into Screen coordinates. Can't be const since plugins compiled to the Rack SDK.
	 * 
	 * @param v The vector in local widget coordinates. Note: cannot change to a reference
	 * since this function is called by plugins there were compiled to the Rack SDK.
	 * @param ancestor The ancestor Widget to transform `v` into the coordinate system of.
	 * @return The vector in absolute/screen coordinates. 
	 */
	virtual math::Vec getRelativeOffset(const math::Vec v, Widget* ancestor);

    /** Returns `v` in this coordinates and transformed into
     * Screen/world/root/global/absolute coordinates.  Note that it is critical
     * to call this method via the class that v is relative to.
     *
     * @param v The vector in local widget coordinates.
     * @return The vector in absolute/screen coordinates.
     */
    math::Vec getInSceneCoords(const math::Vec& v) const {
        return const_cast<Widget*>(this)->getRelativeOffset(v, nullptr);
    }

    /** Returns the zoom level in the coordinate system of `ancestor`.
     * Only `ZoomWidget` should override this to return value other than 1.
     */
    virtual float getRelativeZoom(Widget* ancestor);

	/** Returns the absolute zoom level (relative to the Screen). */
    float getAbsoluteZoom() {
        return getRelativeZoom(nullptr);
    }

    /** Returns a subset of the given Rect bounded by the box of this widget
     * and all ancestors. Does this by doing the transformation for each
     * ancestor up to the Screen.
     */
    virtual math::Rect getViewport(math::Rect r = math::Rect::inf());

    template <class T>
    T* getAncestorOfType() {
        if (!parent_) return NULL;
        T* p = dynamic_cast<T*>(parent_);
        if (p) return p;
        return parent_->getAncestorOfType<T>();
    }

	template <class T>
	T* getFirstDescendantOfType() {
		for (Widget* child : children_) {
			T* c = dynamic_cast<T*>(child);
			if (c)
				return c;
			c = child->getFirstDescendantOfType<T>();
			if (c)
				return c;
		}
		return NULL;
	}

    /** Checks if the given widget is a child of `this` widget.
     */
    bool hasChild(Widget* child);

    /** Const version. Checks if the given widget is a child of `this` widget.
     */
    bool hasChild(Widget* child) const {
		return const_cast<Widget*>(this)->hasChild(child);
	}

    /** Adds widget to the top of the children. Gives ownership of widget to
     * this widget instance.
     */
    void addChild(Widget* child);

    /** Adds widget to the bottom of the children.
     */
    void addChildBottom(Widget* child);

    /** Adds widget directly below another widget.
     * The sibling widget must already be a child of `this` widget.
     */
    void addChildBelow(Widget* child, Widget* sibling);

    /** Adds widget directly above another widget.
     * The sibling widget must already be a child of `this` widget.
     */
    void addChildAbove(Widget* child, Widget* sibling);

    /** Removes widget from list of children if it exists.
    Triggers RemoveEvent of child.
    Does not delete widget but transfers ownership to caller
    */
    void removeChild(Widget* child);

    /** Removes and deletes all child Widgets.
     * Triggers RemoveEvent of all children.
     */
    void clearChildren();

    /** Advances the module by one frame */
    virtual void step();

    struct DrawArgs {
        // The Vector Graphics context to draw to
        NVGcontext* vg = NULL;
        /** Local box representing the visible viewport. */
        math::Rect clipBox;
        NVGLUframebuffer* fb = NULL;
    };

    /** Draws the widget to the NanoVG context.
     * When overriding, call the superclass's `draw(args)` to recurse to
     * children.
     */
    virtual void draw(const DrawArgs& args);

    /** Override draw(const DrawArgs &args) instead */
    DEPRECATED virtual void draw(NVGcontext* vg) {}

    /** Draw additional layers.
     *
     * Custom widgets may draw its children multiple times on different
     * layers, passing an arbitrary layer number each time. Layer 0 calls
     * children's draw(). Layer 1 draws lights and halos. Layer 2 draws
     * plugs. Layer 3 draws cables. When overriding, always wrap draw
     * commands in `if (layer == ...) {}` to avoid drawing on all layers.
     * When overriding, call the superclass's `drawLayer(args, layer)` to
     * recurse to children.
     */
    virtual void drawLayer(const DrawArgs& args, int layer);

    /** Draws a particular child.
     * Saves and restores NanoVG context to prevent changing the given
     * context.
     */
    void drawChild(Widget* child, const DrawArgs& args, int layer = 0);

    // Events

    /** Recurses an event to all visible Widgets */
    template <typename TMethod, class TEvent>
    void recurseEvent(TMethod f, const TEvent& e) {
        for (auto it = children_.rbegin(); it != children_.rend(); it++) {
            // Stop propagation if requested
            if (!e.isPropagating()) break;
            Widget* child = *it;
            // Don't filter child by visibility. Typically only position
            // events need to be filtered by visibility. if
            // (!child->visible) 	continue;

            // Clone event for (currently) no reason
            TEvent e2 = e;
            // Call child event handler
            (child->*f)(e2);
        }
    }

    /** Recurses an event to all visible Widgets until it is consumed. */
	template <typename TMethod, class TEvent>
	void recursePositionEvent(TMethod f, const TEvent& e) {
		for (auto it = children_.rbegin(); it != children_.rend(); it++) {
			// Stop propagation if requested
			if (!e.isPropagating()) break;
			Widget* child = *it;
			// Filter child by visibility and position
			if (!child->visible_) continue;
			if (!child->box_.contains(e.pos)) continue;

			// Clone event and adjust its position
			TEvent e2 = e;
			e2.pos = e.pos.minus(child->getPos());
			// Call child event handler
			(child->*f)(e2);
		}
	}

    using BaseEvent = widget::BaseEvent;

    /** An event prototype with a vector position. */
    struct PositionBaseEvent {
        /** The pixel coordinate where the event occurred, relative to the
         * Widget it is called on. */
        math::Vec pos;
    };

    struct HoverEvent : BaseEvent, PositionBaseEvent {
        /** Change in mouse position since the last frame. Can be zero. */
        math::Vec mouseDelta;
    };

    /** Occurs every frame when the mouse is hovering over a Widget.
     * Recurses. Consume this event to allow Enter and Leave to occur.
     */
    virtual void onHover(const HoverEvent& e) {
        recursePositionEvent(&Widget::onHover, e);
    }

    struct ButtonEvent : BaseEvent, PositionBaseEvent {
        /** GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT,
         * GLFW_MOUSE_BUTTON_MIDDLE, etc. */
        int button;
        /** GLFW_PRESS or GLFW_RELEASE */
        int action;
        /** GLFW_MOD_* */
        int mods;
    };

    /** Occurs each mouse button press or release.
     * Recurses. Consume this event to allow DoubleClick, Select, Deselect,
     * SelectKey, SelectText, DragStart, DragEnd, DragMove, and DragDrop to
     * occur.
     */
    virtual void onButton(const ButtonEvent& e) {
        recursePositionEvent(&Widget::onButton, e);
    }

    struct DoubleClickEvent : BaseEvent {};
    /** Occurs when the left mouse button is pressed a second time on the same
     * Widget within a time duration. Must consume the Button event (on left
     * button press) to receive this event.
     */
	virtual void onDoubleClick(const DoubleClickEvent& e) {
        DEBUG("Unused Widget::onDoubleClick called"); // FIXME
    }

	/** An event prototype with a GLFW key. */
	struct KeyBaseEvent {
		/** The key corresponding to what it would be called in its position on a QWERTY US keyboard.
		For example, the WASD directional keys used for first-person shooters will always be reported as "WASD", regardless if they say "ZQSD" on an AZERTY keyboard.
		You should usually not use these for printable characters such as "Ctrl+V" key commands. Instead, use `keyName`.
		You *should* use these for non-printable keys, such as Escape, arrow keys, Home, F1-12, etc.
		You should also use this for Enter, Tab, and Space. Although they are printable keys, they do not appear in `keyName`.
		See GLFW_KEY_* for the list of possible values.
		*/
		int key;
		/** Platform-dependent "software" key code.
		This variable is only included for completion. There should be no reason for you to use this.
		You should instead use `key` (for non-printable characters) or `keyName` (for printable characters).
		Values are platform independent and can change between different keyboards or keyboard layouts on the same OS.
		*/
		int scancode;
		/** String containing the lowercase key name, if it produces a printable character.
		This is the only variable that correctly represents the label printed on any keyboard layout, whether it's QWERTY, AZERTY, QWERTZ, Dvorak, etc.
		For example, if the user presses the key labeled "q" regardless of the key position, `keyName` will be "q".
		For non-printable characters this is an empty string.
		Enter, Tab, and Space do not give a `keyName`. Use `key` instead.
		Shift has no effect on the key name. Shift+1 results in "1", Shift+q results in "q", etc.
		*/
		std::string keyName;
		/** The type of event occurring with the key.
		Possible values are GLFW_RELEASE, GLFW_PRESS, GLFW_REPEAT, or RACK_HELD.
		RACK_HELD is sent every frame while the key is held.
		*/
		int action;
		/** Bitwise OR of key modifiers, such as Ctrl or Shift.
		Use (mods & RACK_MOD_MASK) == RACK_MOD_CTRL to check for Ctrl on Linux and Windows but Cmd on Mac.
		See GLFW_MOD_* for the list of possible values.
		*/
		int mods;
		/** Checks whether this KeyBaseEvent is the given key command. `mods` is OR'd GLFW modifier keys.
		On Latin keyboards such as QWERTY, AZERTY, QWERTZ, and Dvorak, the key name is checked regardless of position. For example, pressing Q with key=GLFW_KEY_Q returns true for all of these layouts.
		On non-Latin keyboards such as JCUKEN (ЙЦУКЕН), the layout is assumed to match QWERTY. For example, pressing Й with key=GLFW_KEY_Q returns true.
		Implemented in event.cpp.
		*/
		bool isKeyCommand(int key, int mods = 0) const;
	};

    struct HoverKeyEvent : BaseEvent, PositionBaseEvent, KeyBaseEvent {};
    /** Occurs when a key is pressed, released, or repeated while the mouse is
     * hovering a Widget. Recurses.
     */
	virtual void onHoverKey(const HoverKeyEvent& e) {
		recursePositionEvent(&Widget::onHoverKey, e);
	}

    /** An event prototype with a Unicode character. */
    struct TextBaseEvent {
        /** Unicode code point of the character */
        uint32_t codepoint;
    };
    struct HoverTextEvent : BaseEvent, PositionBaseEvent, TextBaseEvent {};

    /** Occurs when a character is typed while the mouse is hovering a Widget.
     * Recurses.
     */
    virtual void onHoverText(const HoverTextEvent& e) {
        recursePositionEvent(&Widget::onHoverText, e);
    }

    struct HoverScrollEvent : BaseEvent, PositionBaseEvent {
        /** Change of scroll wheel position. */
        math::Vec scrollDelta;
    };

    /** Occurs when the mouse scroll wheel is moved while the mouse is hovering
     * a Widget. Recurses.
     */
    virtual void onHoverScroll(const HoverScrollEvent& e) {
        recursePositionEvent(&Widget::onHoverScroll, e);
    }

    struct EnterEvent : BaseEvent {};

    /** Occurs when a Widget begins consuming the Hover event.
     * Must consume the Hover event to receive this event.
     * The target sets `hoveredWidget`, which allows Leave to occur.
     */
    virtual void onEnter(const EnterEvent& e) {}

    struct LeaveEvent : BaseEvent {};

    /** Occurs when a different Widget is entered.
    Must consume the Hover event (when a Widget is entered) to receive this
    event.
    */
    virtual void onLeave(const LeaveEvent& e) {}

    struct SelectEvent : BaseEvent {};

    /** Occurs when a Widget begins consuming the Button press event for the
    left mouse button. Must consume the Button event (on left button press) to
    receive this event. The target sets `selectedWidget`, which allows
    SelectText and SelectKey to occur.
    */
    virtual void onSelect(const SelectEvent& e) {}

    struct DeselectEvent : BaseEvent {};

    /** Occurs when a different Widget is selected.
    Must consume the Button event (on left button press, when the Widget is
    selected) to receive this event.
    */
    virtual void onDeselect(const DeselectEvent& e) {}

    struct SelectKeyEvent : BaseEvent, KeyBaseEvent {};

    /** Occurs when a key is pressed, released, or repeated while a Widget is
    selected. Must consume to prevent HoverKey from being triggered.
    */
    virtual void onSelectKey(const SelectKeyEvent& e) {}

    struct SelectTextEvent : BaseEvent, TextBaseEvent {};

    /** Occurs when text is typed while a Widget is selected.
    Must consume to prevent HoverKey from being triggered.
    */
    virtual void onSelectText(const SelectTextEvent& e) {}

    struct DragBaseEvent : BaseEvent {
        /** The mouse button held while dragging. */
        int button;
    };
    struct DragStartEvent : DragBaseEvent {};

    /** Occurs when a Widget begins being dragged.
    Must consume the Button event (on press) to receive this event.
    The target sets `draggedWidget`, which allows DragEnd, DragMove, DragHover,
    DragEnter, and DragDrop to occur.
    */
    virtual void onDragStart(const DragStartEvent& e) {}

    struct DragEndEvent : DragBaseEvent {};

    /** Occurs when a Widget stops being dragged by releasing the mouse button.
     * Must consume the Button event (on press, when the Widget drag begins) to
     * receive this event.
     */
    virtual void onDragEnd(const DragEndEvent& e) {}

    struct DragMoveEvent : DragBaseEvent {
        /** Change in mouse position since the last frame. Can be zero. */
        math::Vec mouseDelta;
    };

    /** Occurs every frame on the dragged Widget.
     * Must consume the Button event (on press, when the Widget drag begins) to
     * receive this event.
     */
    virtual void onDragMove(const DragMoveEvent& e) {}

    struct DragHoverEvent : DragBaseEvent, PositionBaseEvent {
        /** The dragged widget */
        Widget* origin = NULL;
        /** Change in mouse position since the last frame. Can be zero. */
        math::Vec mouseDelta;
    };

    /** Occurs every frame when the mouse is hovering over a Widget while
     * another Widget (possibly the same one) is being dragged. Recurses.
     * Consume this event to allow DragEnter and DragLeave to occur.
     */
    virtual void onDragHover(const DragHoverEvent& e) {
        recursePositionEvent(&Widget::onDragHover, e);
    }

    struct DragEnterEvent : DragBaseEvent {
        /** The dragged widget */
        Widget* origin = NULL;
    };

    /** Occurs when the mouse enters a Widget while dragging.
     * Must consume the DragHover event to receive this event.
     * The target sets `draggedWidget`, which allows DragLeave to occur.
     */
    virtual void onDragEnter(const DragEnterEvent& e) {}

    struct DragLeaveEvent : DragBaseEvent {
        /** The dragged widget */
        Widget* origin = NULL;
    };

    /** Occurs when the mouse leaves a Widget while dragging.
     * Must consume the DragHover event (when the Widget is entered) to receive
     * this event.
     */
    virtual void onDragLeave(const DragLeaveEvent& e) {}

    struct DragDropEvent : DragBaseEvent {
        /** The dragged widget */
        Widget* origin = NULL;
    };

    /** Occurs when the mouse button is released over a Widget while dragging.
     * Must consume the Button event (on release) to receive this event.
     */
    virtual void onDragDrop(const DragDropEvent& e) {}

    struct PathDropEvent : BaseEvent, PositionBaseEvent {
        PathDropEvent(const std::vector<std::string>& paths) : paths(paths) {}

        /** List of file paths in the dropped selection */
        const std::vector<std::string>& paths;
    };

    /** Occurs when a selection of files from the operating system is dropped
     * onto a Widget. Recurses.
     */
    virtual void onPathDrop(const PathDropEvent& e) {
        recursePositionEvent(&Widget::onPathDrop, e);
    }

    struct ActionEvent : BaseEvent {};

    /** Occurs after a certain action is triggered on a Widget.
    The concept of an "action" is defined by the type of Widget.
    */
    virtual void onAction(const ActionEvent& e) {}

    struct ChangeEvent : BaseEvent {};

    /** Occurs after the value of a Widget changes.
    The concept of a "value" is defined by the type of Widget.
    */
    virtual void onChange(const ChangeEvent& e) {}

    struct DirtyEvent : BaseEvent {};

    /** Occurs when the pixel buffer of this module must be refreshed.
    Recurses.
    */
    virtual void onDirty(const DirtyEvent& e) {
        recurseEvent(&Widget::onDirty, e);
    }

    struct RepositionEvent : BaseEvent {};

    /** Occurs after a Widget's position is set by Widget::setPos().
     */
    virtual void onReposition(const RepositionEvent& e) {}

    struct ResizeEvent : BaseEvent {};

    /** Occurs after a Widget's size is set by Widget::setSize().
     */
    virtual void onResize(const ResizeEvent& e) {}

    struct AddEvent : BaseEvent {};

    /** Occurs after a Widget is added to a parent.
     */
    virtual void onAdd(const AddEvent& e) {}

    struct RemoveEvent : BaseEvent {};

    /** Occurs before a Widget is removed from its parent.
     */
    virtual void onRemove(const RemoveEvent& e) {}

    struct ShowEvent : BaseEvent {};

    /** Occurs after a Widget is shown with Widget::show().
    Recurses.
    */
    virtual void onShow(const ShowEvent& e) {
        recurseEvent(&Widget::onShow, e);
    }

    struct HideEvent : BaseEvent {};

    /** Occurs after a Widget is hidden with Widget::hide().
    Recurses.
    */
    virtual void onHide(const HideEvent& e) {
        recurseEvent(&Widget::onHide, e);
    }

    struct ContextCreateEvent : BaseEvent {
        NVGcontext* vg;
    };

    /** Called after the Window (including OpenGL and NanoVG contexts) are
     * created. Recurses.
     */
    virtual void onContextCreate(const ContextCreateEvent& e) {
        recurseEvent(&Widget::onContextCreate, e);
    }

    struct ContextDestroyEvent : BaseEvent {
        NVGcontext* vg;
    };

    /** Called before the Window (including OpenGL and NanoVG contexts) are
     * destroyed. Recurses.
     */
    virtual void onContextDestroy(const ContextDestroyEvent& e) {
        recurseEvent(&Widget::onContextDestroy, e);
    }

    /** Converts a Scene space vector to local widget coordinates.
	 * If a zoom widdget then zooming is handled by
	 * by a method in ZoomWidget called getScreenVecInLocalCoordsForZoomWidget().
	 * Cannot just add and use a virtual function since plugins compiled to the Rack SDK
	 * and inherited classes that have virtual function would then have corrupted vtable.
	 * @param vec The vector in Scene coordinates.
	 * @return The vector in local widget coordinates.
	 */
	math::Vec getScenePosInLocalCoords(const math::Vec& vec) const;

  private:
	/** Base implementation of getScenePosInLocalCoords without zooming.
	 */
	math::Vec getScenePosInLocalCoordsBase(const math::Vec& vec) const;

   private:
    /** Position relative to parent and size of widget. */
	math::Rect box_ = math::Rect(math::Vec(), math::Vec(INFINITY, INFINITY));

	/** Automatically set when Widget is added as a child to another Widget */
	Widget* parent_ = NULL;

    /** Lazily created children */
	std::list<Widget*> children_;

    /** Disables rendering but allow stepping.
    Use isVisible(), setVisible(), show(), or hide() instead of using this
    variable directly. 
    */
    bool visible_ = true;

	/** If set to true, parent will delete Widget in the next step().
	Use requestDelete() instead of using this variable directly.
	*/
	bool requestedDelete_ = false;
};  // end of class Widget

} // namespace widget

/** Deprecated Rack v1 event namespace.
Use events defined in the widget::Widget class instead of this `event::` namespace in new code.
*/
namespace event {
using Base = widget::BaseEvent;
using PositionBase = widget::Widget::PositionBaseEvent;
using KeyBase = widget::Widget::KeyBaseEvent;
using TextBase = widget::Widget::TextBaseEvent;
using Hover = widget::Widget::HoverEvent;
using Button = widget::Widget::ButtonEvent;
using DoubleClick = widget::Widget::DoubleClickEvent;
using HoverKey = widget::Widget::HoverKeyEvent;
using HoverText = widget::Widget::HoverTextEvent;
using HoverScroll = widget::Widget::HoverScrollEvent;
using Enter = widget::Widget::EnterEvent;
using Leave = widget::Widget::LeaveEvent;
using Select = widget::Widget::SelectEvent;
using Deselect = widget::Widget::DeselectEvent;
using SelectKey = widget::Widget::SelectKeyEvent;
using SelectText = widget::Widget::SelectTextEvent;
using DragBase = widget::Widget::DragBaseEvent;
using DragStart = widget::Widget::DragStartEvent;
using DragEnd = widget::Widget::DragEndEvent;
using DragMove = widget::Widget::DragMoveEvent;
using DragHover = widget::Widget::DragHoverEvent;
using DragEnter = widget::Widget::DragEnterEvent;
using DragLeave = widget::Widget::DragLeaveEvent;
using DragDrop = widget::Widget::DragDropEvent;
using PathDrop = widget::Widget::PathDropEvent;
using Action = widget::Widget::ActionEvent;
using Change = widget::Widget::ChangeEvent;
using Dirty = widget::Widget::DirtyEvent;
using Reposition = widget::Widget::RepositionEvent;
using Resize = widget::Widget::ResizeEvent;
using Add = widget::Widget::AddEvent;
using Remove = widget::Widget::RemoveEvent;
using Show = widget::Widget::ShowEvent;
using Hide = widget::Widget::HideEvent;
}


} // namespace rack
