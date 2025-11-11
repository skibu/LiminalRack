#pragma once
#include <widget/Widget.hpp>


namespace rack {
namespace widget {

/** Caches its children's draw() result to a framebuffer image.
When dirty, its children will be re-rendered on the next call to step().
*/
class FramebufferWidget : public Widget {
   private:
    struct Internal;
    Internal* internal_;

    bool dirty_ = true;
    bool bypassed_ = false;
    float oversample_ = 1.0;

    /** Redraw when the world offset of the FramebufferWidget changes its
     * fractional value. */
    bool dirtyOnSubpixelChange_ = true;

    /** If finite, the maximum size of the framebuffer is the viewport expanded
     * by this margin. The framebuffer is re-rendered when the viewport moves
     * outside the margin. */
    math::Vec viewportMargin_ = math::Vec(INFINITY, INFINITY);

   public:
    FramebufferWidget();
    ~FramebufferWidget();

    /** Sets the oversampling factor for the framebuffer. */
    void setOversample(float oversample) {
        oversample_ = oversample;
    }

    /** Sets whether to redraw when the world offset of the FramebufferWidget
     * changes its fractional value. Default true. */
    void setDirtyOnSubpixelChange(bool dirty) {
        dirtyOnSubpixelChange_ = dirty;
    }

    /** Requests to re-render children to the framebuffer on the next draw(). */
    void setDirty(bool dirty = true);

    int getImageHandle();
    NVGLUframebuffer* getFramebuffer();
    math::Vec getFramebufferSize();
    void deleteFramebuffer();

    void step() override;

    /** Draws the framebuffer to the NanoVG scene, re-rendering it if necessary.
     */
    void draw(const DrawArgs& args) override;

    /** Re-renders the framebuffer, re-creating it if necessary.
     * Handles oversampling (if >1) by rendering to a temporary (larger)
     * framebuffer and then downscaling it to the main persistent framebuffer.
     */
    void render(math::Vec scale = math::Vec(1, 1),
                math::Vec offsetF = math::Vec(0, 0),
                math::Rect clipBox = math::Rect::inf());

    /** Initializes the current GL context and draws children to it. */
    virtual void drawFramebuffer();

    /** Called when the framebuffer needs to be marked as dirty. */
    void onDirty(const DirtyEvent& e) override;

    /** Called when Scene is created and can be written to. Marks framebuffer as
     * dirty. */
    void onContextCreate(const ContextCreateEvent& e) override;

    /** Called when Scene is being destroyed. Deletes framebuffer. Marks
     * framebuffer as dirty. */
    void onContextDestroy(const ContextDestroyEvent& e) override;
};

} // namespace widget
} // namespace rack
