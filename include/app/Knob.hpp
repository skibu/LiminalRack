#pragma once
#include <app/common.hpp>
#include <app/ParamWidget.hpp>
#include <context.hpp>


namespace rack {
namespace app {

/** Implements knob interface for dragging and clicking behavior for a
 * ParamWidgets */
struct Knob : ParamWidget {
    // Internal data structure private because it is internal!
   private:
    struct Internal;
    Internal* internal_;

   public:
    /** Drag horizontally instead of vertically. */
    bool horizontal = false;

    /** Enables per-sample value smoothing while dragging.
    Alternatively, use ParamQuantity::smoothEnabled.
    */
    bool smooth = true;

    /** Enables value snapping to the nearest integer.
    Alternatively, use ParamQuantity::snapEnabled.
    */
    bool snap = false;

    /** Multiplier for mouse movement to adjust knob value */
    float speed = 1.f;

    /** Force dragging to linear, e.g. for sliders. */
    bool forceLinear = false;

    /** Angles in radians.
    For drawing and handling the global radial knob setting.
    */
    float minAngle = -M_PI;
    float maxAngle = M_PI;

    Knob();
    ~Knob();
    void initParamQuantity() override;

    void onHover(const HoverEvent& e) override;

    /** Determine if mouse position is in the circle (for x direction).
     * If so, call ParamWidget::onButton().
     */
    void onButton(const ButtonEvent& e) override;

    void onDragStart(const DragStartEvent& e) override;
    void onDragEnd(const DragEndEvent& e) override;
    void onDragMove(const DragMoveEvent& e) override;
    void onDragLeave(const DragLeaveEvent& e) override;
    void onHoverScroll(const HoverScrollEvent& e) override;
    void onLeave(const LeaveEvent& e) override;

    // Want to override draw() to add 3D effects. But since 3rd-party plugins
    // are compiled against the legacy Rack SDK, they will not call an overriden
    // function. So we create a new function drawOverride() that does the work,
    // and call that from ParamWidget::draw().
    void drawOverride(const DrawArgs& args);

   private:
    // Draw 3D effects such as shadows
    void draw3DEffects(const DrawArgs& args);

    // Highlights knob if dragging it
    void drawHighlight(const DrawArgs& args);

    /** Called when user clicks the knob without moving it.
    Useful for handling emulating push-knobs in hardware.
    */
    void onAction(const ActionEvent& e) override {}
};

} // namespace app
} // namespace rack
