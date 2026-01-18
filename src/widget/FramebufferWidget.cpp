#include <widget/FramebufferWidget.hpp>
#include <context.hpp>
#include <random.hpp>


namespace rack {
namespace widget {


static int FramebufferWidget_totalPixels = 0;

struct FramebufferWidget::Internal {
    NVGLUframebuffer* fb_ = NULL;

    /** Pixel dimensions of the allocated framebuffer */
    math::Vec fbSize_;

    /** Bounding box in world coordinates of where the framebuffer should be
    painted. Always has integer coordinates so that blitting framebuffers is
    pixel-perfect. */
    math::Rect fbBox_;

    /** Framebuffer's scale relative to the world */
    math::Vec fbScale_;

    /** Framebuffer's subpixel offset relative to fbBox_ in world coordinates */
    math::Vec fbOffsetF_;

    /** Local box where framebuffer content is valid.*/
    math::Rect fbClipBox_ = math::Rect::inf();
};

FramebufferWidget::FramebufferWidget() {
	internal_ = new Internal;
}


FramebufferWidget::~FramebufferWidget() {
	deleteFramebuffer();
	delete internal_;
}


void FramebufferWidget::setDirty(bool dirty) {
	this->dirty_ = dirty;
}


int FramebufferWidget::getImageHandle() {
	if (!internal_->fb_)
		return -1;
	return internal_->fb_->image;
}


NVGLUframebuffer* FramebufferWidget::getFramebuffer() {
	return internal_->fb_;
}


math::Vec FramebufferWidget::getFramebufferSize() {
	return internal_->fbSize_;
}


void FramebufferWidget::deleteFramebuffer() {
	if (!internal_->fb_)
		return;

	// If the framebuffer exists, the Window should exist.
	assert(getWindow());

	nvgluDeleteFramebuffer(internal_->fb_);
	internal_->fb_ = NULL;

	FramebufferWidget_totalPixels -= internal_->fbSize_.area();
}


void FramebufferWidget::step() {
	Widget::step();
}


void FramebufferWidget::draw(const DrawArgs& args) {
	// Draw directly if bypassed or already drawing in a framebuffer
	if (bypassed_ || args.fb) {
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
	math::Vec offsetFDelta = offsetF.minus(internal_->fbOffsetF_);
	if (dirtyOnSubpixelChange_ && getWindow()->fbDirtyOnSubpixelChange() && offsetFDelta.square() >= std::pow(0.1f, 2)) {
		//TRACE("%p dirty subpixel (%f, %f) (%f, %f)", this, VEC_ARGS(offsetF), VEC_ARGS(internal_->fbOffsetF_));
		setDirty();
	}
	// Re-render if rescaled.
	else if (!scale.equals(internal_->fbScale_)) {
		//TRACE("%p dirty scale", this);
		setDirty();
	}
	// Re-render if viewport is outside framebuffer's clipbox when it was rendered.
	else if (!internal_->fbClipBox_.contains(args.clipBox)) {
		setDirty();
	}

	if (dirty_) {
		// Render only if there is frame time remaining (to avoid lagging frames significantly), or if it's one of the first framebuffers this frame (to avoid framebuffers from never rendering).
		const int minCount = 1;
		const double minRemaining = -1 / 60.0;
		int count = ++getWindow()->fbCount();
		double remaining = getWindow()->getFrameDurationRemaining();
		if (count <= minCount || remaining > minRemaining) {
			render(scale, offsetF, args.clipBox);
		}
	}

	if (!internal_->fb_)
		return;

	// Draw framebuffer image, using world coordinates
	nvgSave(args.vg);
	nvgResetTransform(args.vg);

	math::Vec scaleRatio = scale.div(internal_->fbScale_);
	//TRACE("%f %f %f %f", VEC_ARGS(scaleRatio), VEC_ARGS(offsetF));

	//TRACE("%f %f %f %f, %f %f", RECT_ARGS(internal_->fbBox_), VEC_ARGS(internal_->fbSize_));
	//TRACE("offsetI (%f, %f) fbBox_ (%f, %f; %f, %f)", VEC_ARGS(offsetI), RECT_ARGS(internal_->fbBox_));
	nvgBeginPath(args.vg);
	nvgRect(args.vg,
		offsetI.getX() + internal_->fbBox_.getX() * scaleRatio.getX(),
		offsetI.getY() + internal_->fbBox_.getY() * scaleRatio.getY(),
		internal_->fbBox_.getWidth() * scaleRatio.getX(),
		internal_->fbBox_.getHeight() * scaleRatio.getY());

	NVGpaint paint = nvgImagePattern(args.vg,
		offsetI.getX() + internal_->fbBox_.getX() * scaleRatio.getX(),
		offsetI.getY() + internal_->fbBox_.getY() * scaleRatio.getY(),
		internal_->fbBox_.getWidth() * scaleRatio.getX(),
		internal_->fbBox_.getHeight() * scaleRatio.getY(),
		0.0, internal_->fb_->image, 1.0);
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
    dirty_ = false;
    NVGcontext* vg = getWindow()->vg_;
    NVGcontext* fbVg = getWindow()->fbVg_;

    internal_->fbScale_ = scale;
    internal_->fbOffsetF_ = offsetF;

    math::Rect localBox;
    if (getChildren().empty()) {
        localBox = getBox().zeroPos();
    } else {
        localBox = getVisibleChildrenBoundingBox();
    }

    // Intersect local box with viewport if viewportMargin_ is set
    internal_->fbClipBox_ = clipBox.grow(viewportMargin_);
    if (internal_->fbClipBox_.getSize().isFinite()) {
        localBox = localBox.intersect(internal_->fbClipBox_);
    }

    // TRACE(
    //     "rendering FramebufferWidget localBox (%f, %f; %f, %f) fbOffset (%f, "
    //     "%f) fbScale_ (%f, %f)",
    //     RECT_ARGS(localBox), VEC_ARGS(internal_->fbOffsetF_),
    //     VEC_ARGS(internal_->fbScale_));
    // Transform to world coordinates, then expand to nearest integer coordinates
    math::Vec min = localBox.getTopLeft()
                        .mult(internal_->fbScale_)
                        .plus(internal_->fbOffsetF_)
                        .floor();
    math::Vec max = localBox.getBottomRight()
                        .mult(internal_->fbScale_)
                        .plus(internal_->fbOffsetF_)
                        .ceil();
    internal_->fbBox_ = math::Rect::fromMinMax(min, max);
    //TRACE("%g %g %g %g", RECT_ARGS(internal_->fbBox_));

    float pixelRatio = std::fmax(1.f, std::floor(getWindow()->pixelRatio_));
    math::Vec newFbSize = internal_->fbBox_.getSize().mult(pixelRatio).ceil();

    // Create framebuffer if a new size is needed
    if (!internal_->fb_ || !newFbSize.equals(internal_->fbSize_)) {
        // Delete old framebuffer
        deleteFramebuffer();

        // Create a framebuffer
        if (newFbSize.isFinite() && !newFbSize.isZero()) {
            //TRACE("Creating framebuffer of size (%f, %f)", VEC_ARGS(newFbSize));
            internal_->fb_ =
                nvgluCreateFramebuffer(vg, newFbSize.getX(), newFbSize.getY(), 0);
            FramebufferWidget_totalPixels += newFbSize.area();
        }

        //TRACE("Framebuffer total pixels: %.1f Mpx", FramebufferWidget_totalPixels / 1e6);
        internal_->fbSize_ = newFbSize;
    }
    if (!internal_->fb_) {
        WARN(
            "Framebuffer of size (%f, %f) could not be created for "
            "FramebufferWidget %p.",
            VEC_ARGS(internal_->fbSize_), this);
        return;
    }

    //TRACE("Drawing to framebuffer of size (%f, %f)", VEC_ARGS(internal_->fbSize_));

    // Render to framebuffer
    if (oversample_ == 1.0) {
        // If not oversampling, render directly to framebuffer.
        nvgluBindFramebuffer(internal_->fb_);
        drawFramebuffer();
        nvgluBindFramebuffer(NULL);
    } else {
        NVGLUframebuffer* fb = internal_->fb_;
        // If oversampling, create another framebuffer and copy it to actual
        // size.
        math::Vec oversampledFbSize = internal_->fbSize_.mult(oversample_).ceil();
        // TRACE("Creating %0.fx oversampled framebuffer of size (%f, %f)",
        //       oversample_, VEC_ARGS(internal_->fbSize_));
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
        internal_->fb_ = oversampledFb;
        drawFramebuffer();
        internal_->fb_ = fb;
        nvgluBindFramebuffer(NULL);

        // Use NanoVG for copying oversampled framebuffer to normal framebuffer
        nvgluBindFramebuffer(internal_->fb_);
        nvgBeginFrame(fbVg, internal_->fbBox_.getWidth(), internal_->fbBox_.getHeight(),
                      1.0);

        // Draw oversampled framebuffer
        nvgBeginPath(fbVg);
        nvgRect(fbVg, 0.0, 0.0, internal_->fbSize_.getX(), internal_->fbSize_.getY());
        NVGpaint paint =
            nvgImagePattern(fbVg, 0.0, 0.0, internal_->fbSize_.getX(),
                            internal_->fbSize_.getY(), 0.0, oversampledFb->image, 1.0);
        nvgFillPaint(fbVg, paint);
        nvgFill(fbVg);

        glViewport(0.0, 0.0, internal_->fbSize_.getX(), internal_->fbSize_.getY());
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
        internal_->fbSize_.getX() * oversample_ / internal_->fbBox_.getWidth();
    nvgBeginFrame(vg, internal_->fbBox_.getWidth(), internal_->fbBox_.getHeight(),
                  pixelRatio);

    // Use local scaling
    nvgTranslate(vg, -internal_->fbBox_.getX(), -internal_->fbBox_.getY());
    nvgTranslate(vg, internal_->fbOffsetF_.getX(), internal_->fbOffsetF_.getY());
    nvgScale(vg, internal_->fbScale_.getX(), internal_->fbScale_.getY());

    // Draw children
    DrawArgs args;
    args.vg = vg;
    args.clipBox = getBox().zeroPos();
    args.fb = internal_->fb_;
    Widget::draw(args);

    glViewport(0.0, 0.0, internal_->fbSize_.getX() * oversample_,
               internal_->fbSize_.getY() * oversample_);
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
