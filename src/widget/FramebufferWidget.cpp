#include <widget/FramebufferWidget.hpp>
#include <context.hpp>
#include <random.hpp>


namespace rack {
namespace widget {


static int FramebufferWidget_totalPixels = 0;


struct FramebufferWidget::Internal {
	NVGLUframebuffer* fb = NULL;

	/** Pixel dimensions of the allocated framebuffer */
	math::Vec fbSize;
	/** Bounding box in world coordinates of where the framebuffer should be painted.
	Always has integer coordinates so that blitting framebuffers is pixel-perfect.
	*/
	math::Rect fbBox;
	/** Framebuffer's scale relative to the world */
	math::Vec fbScale;
	/** Framebuffer's subpixel offset relative to fbBox in world coordinates */
	math::Vec fbOffsetF;
	/** Local box where framebuffer content is valid.
	*/
	math::Rect fbClipBox = math::Rect::inf();
};


FramebufferWidget::FramebufferWidget() {
	internal_ = new Internal;
}


FramebufferWidget::~FramebufferWidget() {
	deleteFramebuffer();
	delete internal_;
}


void FramebufferWidget::setDirty(bool dirty) {
	this->dirty = dirty;
}


int FramebufferWidget::getImageHandle() {
	if (!internal_->fb)
		return -1;
	return internal_->fb->image;
}


NVGLUframebuffer* FramebufferWidget::getFramebuffer() {
	return internal_->fb;
}


math::Vec FramebufferWidget::getFramebufferSize() {
	return internal_->fbSize;
}


void FramebufferWidget::deleteFramebuffer() {
	if (!internal_->fb)
		return;

	// If the framebuffer exists, the Window should exist.
	assert(getWindow());

	nvgluDeleteFramebuffer(internal_->fb);
	internal_->fb = NULL;

	FramebufferWidget_totalPixels -= internal_->fbSize.area();
}


void FramebufferWidget::step() {
	Widget::step();
}


void FramebufferWidget::draw(const DrawArgs& args) {
	// Draw directly if bypassed or already drawing in a framebuffer
	if (bypassed || args.fb) {
		Widget::draw(args);
		return;
	}

	// Get world transform
	float xform[6];
	nvgCurrentTransform(args.vg, xform);
	// Skew and rotate is not supported
	if (!math::isNear(xform[1], 0.f) || !math::isNear(xform[2], 0.f)) {
		WARN("Skew and rotation detected but not supported in FramebufferWidget.");
		return;
	}

	// Extract scale and offset from world transform
	math::Vec scale = math::Vec(xform[0], xform[3]);
	math::Vec offset = math::Vec(xform[4], xform[5]);
	math::Vec offsetI = offset.floor();
	math::Vec offsetF = offset.minus(offsetI);

	// Re-render if drawing to a new subpixel location.
	// Anything less than 0.1 pixels isn't noticeable.
	math::Vec offsetFDelta = offsetF.minus(internal_->fbOffsetF);
	if (dirtyOnSubpixelChange && getWindow()->fbDirtyOnSubpixelChange() && offsetFDelta.square() >= std::pow(0.1f, 2)) {
		TRACE("%p dirty subpixel (%f, %f) (%f, %f)", this, VEC_ARGS(offsetF), VEC_ARGS(internal_->fbOffsetF));
		setDirty();
	}
	// Re-render if rescaled.
	else if (!scale.equals(internal_->fbScale)) {
		TRACE("%p dirty scale", this);
		setDirty();
	}
	// Re-render if viewport is outside framebuffer's clipbox when it was rendered.
	else if (!internal_->fbClipBox.contains(args.clipBox)) {
		setDirty();
	}

	if (dirty) {
		// Render only if there is frame time remaining (to avoid lagging frames significantly), or if it's one of the first framebuffers this frame (to avoid framebuffers from never rendering).
		const int minCount = 1;
		const double minRemaining = -1 / 60.0;
		int count = ++getWindow()->fbCount();
		double remaining = getWindow()->getFrameDurationRemaining();
		if (count <= minCount || remaining > minRemaining) {
			render(scale, offsetF, args.clipBox);
		}
	}

	if (!internal_->fb)
		return;

	// Draw framebuffer image, using world coordinates
	nvgSave(args.vg);
	nvgResetTransform(args.vg);

	math::Vec scaleRatio = scale.div(internal_->fbScale);
	TRACE("%f %f %f %f", VEC_ARGS(scaleRatio), VEC_ARGS(offsetF));

	TRACE("%f %f %f %f, %f %f", RECT_ARGS(internal_->fbBox), VEC_ARGS(internal_->fbSize));
	TRACE("offsetI (%f, %f) fbBox (%f, %f; %f, %f)", VEC_ARGS(offsetI), RECT_ARGS(internal_->fbBox));
	nvgBeginPath(args.vg);
	nvgRect(args.vg,
		offsetI.getX() + internal_->fbBox.getX() * scaleRatio.getX(),
		offsetI.getY() + internal_->fbBox.getY() * scaleRatio.getY(),
		internal_->fbBox.getWidth() * scaleRatio.getX(),
		internal_->fbBox.getHeight() * scaleRatio.getY());

	NVGpaint paint = nvgImagePattern(args.vg,
		offsetI.getX() + internal_->fbBox.getX() * scaleRatio.getX(),
		offsetI.getY() + internal_->fbBox.getY() * scaleRatio.getY(),
		internal_->fbBox.getWidth() * scaleRatio.getX(),
		internal_->fbBox.getHeight() * scaleRatio.getY(),
		0.0, internal_->fb->image, 1.0);
	nvgFillPaint(args.vg, paint);
	nvgFill(args.vg);

	// For debugging the bounding box of the framebuffer
	// nvgStrokeWidth(args.vg, 2.0);
	// nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 0, 0.5));
	// nvgStroke(args.vg);

	nvgRestore(args.vg);
}

void FramebufferWidget::render(math::Vec scale, math::Vec offsetF,
                               math::Rect clipBox) {
    // In case we fail drawing the framebuffer, don't try again the next frame,
    // so reset `dirty` here.
    dirty = false;
    NVGcontext* vg = getWindow()->vg_;
    NVGcontext* fbVg = getWindow()->fbVg_;

    internal_->fbScale = scale;
    internal_->fbOffsetF = offsetF;

    math::Rect localBox;
    if (getChildren().empty()) {
        localBox = getBox().zeroPos();
    } else {
        localBox = getVisibleChildrenBoundingBox();
    }

    // Intersect local box with viewport if viewportMargin is set
    internal_->fbClipBox = clipBox.grow(viewportMargin);
    if (internal_->fbClipBox.getSize().isFinite()) {
        localBox = localBox.intersect(internal_->fbClipBox);
    }

    TRACE(
        "rendering FramebufferWidget localBox (%f, %f; %f, %f) fbOffset (%f, "
        "%f) fbScale (%f, %f)",
        RECT_ARGS(localBox), VEC_ARGS(internal_->fbOffsetF),
        VEC_ARGS(internal_->fbScale));
    // Transform to world coordinates, then expand to nearest integer coordinates
    math::Vec min = localBox.getTopLeft()
                        .mult(internal_->fbScale)
                        .plus(internal_->fbOffsetF)
                        .floor();
    math::Vec max = localBox.getBottomRight()
                        .mult(internal_->fbScale)
                        .plus(internal_->fbOffsetF)
                        .ceil();
    internal_->fbBox = math::Rect::fromMinMax(min, max);
    TRACE("%g %g %g %g", RECT_ARGS(internal_->fbBox));

    float pixelRatio = std::fmax(1.f, std::floor(getWindow()->pixelRatio_));
    math::Vec newFbSize = internal_->fbBox.getSize().mult(pixelRatio).ceil();

    // Create framebuffer if a new size is needed
    if (!internal_->fb || !newFbSize.equals(internal_->fbSize)) {
        // Delete old framebuffer
        deleteFramebuffer();

        // Create a framebuffer
        if (newFbSize.isFinite() && !newFbSize.isZero()) {
            TRACE("Creating framebuffer of size (%f, %f)", VEC_ARGS(newFbSize));
            internal_->fb =
                nvgluCreateFramebuffer(vg, newFbSize.getX(), newFbSize.getY(), 0);
            FramebufferWidget_totalPixels += newFbSize.area();
        }

        TRACE("Framebuffer total pixels: %.1f Mpx", FramebufferWidget_totalPixels / 1e6);
        internal_->fbSize = newFbSize;
    }
    if (!internal_->fb) {
        WARN(
            "Framebuffer of size (%f, %f) could not be created for "
            "FramebufferWidget %p.",
            VEC_ARGS(internal_->fbSize), this);
        return;
    }

    TRACE("Drawing to framebuffer of size (%f, %f)", VEC_ARGS(internal_->fbSize));

    // Render to framebuffer
    if (oversample == 1.0) {
        // If not oversampling, render directly to framebuffer.
        nvgluBindFramebuffer(internal_->fb);
        drawFramebuffer();
        nvgluBindFramebuffer(NULL);
    } else {
        NVGLUframebuffer* fb = internal_->fb;
        // If oversampling, create another framebuffer and copy it to actual
        // size.
        math::Vec oversampledFbSize = internal_->fbSize.mult(oversample).ceil();
        TRACE("Creating %0.fx oversampled framebuffer of size (%f, %f)",
              oversample, VEC_ARGS(internal_->fbSize));
        NVGLUframebuffer* oversampledFb = nvgluCreateFramebuffer(
            fbVg, oversampledFbSize.getX(), oversampledFbSize.getY(), 0);

        if (!oversampledFb) {
            WARN(
                "Oversampled framebuffer of size (%f, %f) could not be created "
                "for FramebufferWidget %p.",
                VEC_ARGS(oversampledFbSize), this);
            return;
        }

        // Render to oversampled framebuffer.
        nvgluBindFramebuffer(oversampledFb);
        internal_->fb = oversampledFb;
        drawFramebuffer();
        internal_->fb = fb;
        nvgluBindFramebuffer(NULL);

        // Use NanoVG for copying oversampled framebuffer to normal framebuffer
        nvgluBindFramebuffer(internal_->fb);
        nvgBeginFrame(fbVg, internal_->fbBox.getWidth(), internal_->fbBox.getHeight(),
                      1.0);

        // Draw oversampled framebuffer
        nvgBeginPath(fbVg);
        nvgRect(fbVg, 0.0, 0.0, internal_->fbSize.getX(), internal_->fbSize.getY());
        NVGpaint paint =
            nvgImagePattern(fbVg, 0.0, 0.0, internal_->fbSize.getX(),
                            internal_->fbSize.getY(), 0.0, oversampledFb->image, 1.0);
        nvgFillPaint(fbVg, paint);
        nvgFill(fbVg);

        glViewport(0.0, 0.0, internal_->fbSize.getX(), internal_->fbSize.getY());
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);
        nvgEndFrame(fbVg);
        nvgReset(fbVg);

        nvgluBindFramebuffer(NULL);
        nvgluDeleteFramebuffer(oversampledFb);
    }
};

void FramebufferWidget::drawFramebuffer() {
    NVGcontext* vg = getWindow()->fbVg_;
    nvgSave(vg);

    float pixelRatio =
        internal_->fbSize.getX() * oversample / internal_->fbBox.getWidth();
    nvgBeginFrame(vg, internal_->fbBox.getWidth(), internal_->fbBox.getHeight(),
                  pixelRatio);

    // Use local scaling
    nvgTranslate(vg, -internal_->fbBox.getX(), -internal_->fbBox.getY());
    nvgTranslate(vg, internal_->fbOffsetF.getX(), internal_->fbOffsetF.getY());
    nvgScale(vg, internal_->fbScale.getX(), internal_->fbScale.getY());

    // Draw children
    DrawArgs args;
    args.vg = vg;
    args.clipBox = getBox().zeroPos();
    args.fb = internal_->fb;
    Widget::draw(args);

    glViewport(0.0, 0.0, internal_->fbSize.getX() * oversample,
               internal_->fbSize.getY() * oversample);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    // glClearColor(0.0, 1.0, 1.0, 0.5);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgEndFrame(vg);

    // Clean up the NanoVG state so that calls to nvgTextBounds() etc during
    // step() don't use a dirty state.
    nvgReset(vg);
    nvgRestore(vg);
}

void FramebufferWidget::onDirty(const DirtyEvent& e) {
	setDirty();
	Widget::onDirty(e);
}


void FramebufferWidget::onContextCreate(const ContextCreateEvent& e) {
	setDirty();
	Widget::onContextCreate(e);
}


void FramebufferWidget::onContextDestroy(const ContextDestroyEvent& e) {
	deleteFramebuffer();
	setDirty();
	Widget::onContextDestroy(e);
}


} // namespace widget
} // namespace rack
