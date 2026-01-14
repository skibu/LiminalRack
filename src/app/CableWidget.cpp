#include <cstdlib> 
#include <app/CableWidget.hpp>
#include <app/CableColorMatcher.hpp>
#include <widget/SvgWidget.hpp>
#include <widget/TransformWidget.hpp>
#include <app/Scene.hpp>
#include <app/RackWidget.hpp>
#include <app/ModuleWidget.hpp>
#include <context.hpp>
#include <asset.hpp>
#include <settings.hpp>
#include <engine/Engine.hpp>
#include <engine/Port.hpp>
#include <app/MultiLightWidget.hpp>
#include <componentlibrary.hpp>


namespace rack {
namespace app {


struct TintWidget : widget::Widget {
	// The color to tint the widget with. WHITE is just the
	// initial default. Typically this is set by CableWidget.
	NVGcolor color = color::WHITE;

	void draw(const DrawArgs& args) override {
		nvgTint(args.vg, color);
		Widget::draw(args);
	}
};


/** 
 * To draw a light indicating state of the plug. Will be a 
 * combination of red, green, and blue lights.
 * Color is specified by a TintWidget.
 */
struct PlugLight : componentlibrary::TRedGreenBlueLight<app::MultiLightWidget> {
	PlugLight() {
		setSize(math::Vec(9, 9));
	}
};


struct PlugWidget::PlugInternals {
	// The parent cable widget.
	CableWidget* cableWidget;

	// Type of the plug, either INPUT or OUTPUT.
	engine::Port::Type type;

	// Angle of the plug. Initially pointing upward. 
	float angle = 0.5f * M_PI;

	// The framebuffer used to draw the plug.
	widget::FramebufferWidget* fb;

	// How to rotate and scale the drawing of the plug
	widget::TransformWidget* plugTransform;

	// Color of the plug, set by CableWidget.
	TintWidget* plugTint;

	// Drawing of the cable's plug
	widget::SvgWidget* plug;

	// Drawing of the jack
	widget::SvgWidget* plugPort;

	// Drawing of an light indicating state/voltage of the jack
	app::MultiLightWidget* plugLight;
};


PlugWidget::PlugWidget(const CableWidget* cableWidget, engine::Port::Type type) {
    plugInternals = new PlugInternals;

    plugInternals->cableWidget = const_cast<CableWidget*>(cableWidget);
    plugInternals->type = type;

    plugInternals->fb = new widget::FramebufferWidget;
    addChild(plugInternals->fb);

    plugInternals->plugTransform = new widget::TransformWidget;
    plugInternals->fb->addChild(plugInternals->plugTransform);

    plugInternals->plugTint = new TintWidget;
    plugInternals->plugTransform->addChild(plugInternals->plugTint);

    // Setup for drawing of the jack
    plugInternals->plugPort = new widget::SvgWidget;
    plugInternals->plugPort->setSvg(
        window::Svg::load(asset::system("res/ComponentLibrary/PlugPort.svg")));
    plugInternals->plugPort->setPos(plugInternals->plugPort->getSize().mult(-0.5));
    plugInternals->fb->addChild(plugInternals->plugPort);

    // Setup for drawing of the plug
    plugInternals->plug = new widget::SvgWidget;
	auto plus_svg_filename = plugInternals->type == engine::Port::INPUT ? 
		"res/ComponentLibrary/PlugInput.svg" : "res/ComponentLibrary/PlugOutput.svg";
    plugInternals->plug->setSvg(window::Svg::load(asset::system(plus_svg_filename)));
    plugInternals->plugTint->addChild(plugInternals->plug);
    plugInternals->plugTransform->setSize(plugInternals->plug->getSize());
    plugInternals->plugTransform->setPos(plugInternals->plug->getSize().mult(-0.5));
    plugInternals->plugTint->setSize(plugInternals->plug->getSize());

	// Setup for drawing of the light indicating state/voltage of the plug
    plugInternals->plugLight = new PlugLight;
    plugInternals->plugLight->setPos(plugInternals->plugLight->getSize().mult(-0.5));
    addChild(plugInternals->plugLight);

    setSize(plugInternals->plug->getSize());
}

PlugWidget::~PlugWidget() {
	delete plugInternals;
}

void PlugWidget::step() {
	// To contain brightness of colors (red, green, and blue) of the plug light
	std::vector<float> values(3);

	// Gets info from the port on brightness for each color. Stores those values
	// in the plug's plugLight so that it can be drawn later
	PortWidget* pw = plugInternals->cableWidget->getPort(plugInternals->type);
	if (pw && plugInternals->plugLight->isVisible()) {
		engine::Port* port = pw->getPort();
		if (port) {
			for (int i = 0; i < 3; i++) {
				values[i] = port->plugLights[i].getBrightness();
			}
		}
	}
	plugInternals->plugLight->setBrightnesses(values);

	Widget::step();
}

void PlugWidget::setColor(NVGcolor color) {
	if (color::isEqual(color, plugInternals->plugTint->color))
		return;
	plugInternals->plugTint->color = color;
	plugInternals->fb->setDirty();
}

void PlugWidget::setAngle(float angle) {
	if (angle == plugInternals->angle)
		return;
	plugInternals->angle = angle;
	plugInternals->plugTransform->identity();
	plugInternals->plugTransform->rotate(angle - 0.5f * M_PI, plugInternals->plug->getSize().div(2));
	plugInternals->fb->setDirty();
}

void PlugWidget::setTop(bool top) {
	plugInternals->plugLight->setVisible(top);
}

CableWidget* PlugWidget::getCable() {
	return plugInternals->cableWidget;
}

engine::Port::Type PlugWidget::getType() {
	return plugInternals->type;
}


struct CableWidget::CableInternal {
	/** For making history consistent when disconnecting and reconnecting cable. */
	int64_t cableId = -1;
};


CableWidget::CableWidget() {
	cableInternals_ = new CableInternal;
	color_ = color::BLACK_TRANSPARENT;
    tensionRandomValue_ = generateTensionRandomValue();  // Value between -1.0 and 1.0
	outputPlug_ = new PlugWidget(this, engine::Port::OUTPUT);
	inputPlug_ = new PlugWidget(this, engine::Port::INPUT);
}


CableWidget::~CableWidget() {
	delete outputPlug_;
	delete inputPlug_;

	setCable(NULL);
	delete cableInternals_;
}

float CableWidget::generateTensionRandomValue() {
    float min_val = -1.0f;
    float max_val = 1.0f;
    float random_float_range =
        min_val + static_cast<float>(rand()) / RAND_MAX * (max_val - min_val);

    return random_float_range;
}

bool CableWidget::isComplete() {
	return outputPort_ && inputPort_;
}


void CableWidget::updateCable() {
	// Clean up existing cable if it exists
	if (cable_) {
		getEngine()->removeCable(cable_);
		delete cable_;
		cable_ = NULL;
	}

	// If either input or output port not set then cannot create cable
	if (!inputPort_ || !outputPort_) return;

	// Both ports are set, create a new cable
	cable_ = new engine::Cable;
	cable_->id = cableInternals_->cableId;
	cable_->inputModule = inputPort_->module;
	cable_->inputId = inputPort_->portId;
	cable_->outputModule = outputPort_->module;
	cable_->outputId = outputPort_->portId;
	getEngine()->addCable(cable_);
	cableInternals_->cableId = cable_->id;

	// Make sure cable color is correct. It was originally set based
	// on the first port, but that port might not have definitively
	// determined the color, perhaps because name of the port had
	// insufficient information.
    color_ = CableColorMatcher::getCableColor(inputPort_->getPortInfo(),
											outputPort_->getPortInfo());
}

void CableWidget::setCable(engine::Cable* cable) {
	if (this->cable_) {
		getEngine()->removeCable(this->cable_);
		delete this->cable_;
		this->cable_ = NULL;
		cableInternals_->cableId = -1;
	}
	if (cable) {
		app::ModuleWidget* outputMw = getRack()->getModule(cable->outputModule->id);
		if (!outputMw)
			throw Exception("Cable cannot find output ModuleWidget %lld", (long long) cable->outputModule->id);
		outputPort_ = outputMw->getOutput(cable->outputId);
		if (!outputPort_)
			throw Exception("Cable cannot find output port %d", cable->outputId);

		app::ModuleWidget* inputMw = getRack()->getModule(cable->inputModule->id);
		if (!inputMw)
			throw Exception("Cable cannot find input ModuleWidget %lld", (long long) cable->inputModule->id);
		inputPort_ = inputMw->getInput(cable->inputId);
		if (!inputPort_)
			throw Exception("Cable cannot find input port %d", cable->inputId);

		this->cable_ = cable;
		cableInternals_->cableId = cable->id;
	}
	else {
		outputPort_ = NULL;
		inputPort_ = NULL;
	}
}


engine::Cable* CableWidget::getCable() {
	return cable_;
}

math::Vec CableWidget::getInputPortPosInRackCoords() const {
    if (inputPort_) {
        // Return center of the input port in Rack coordinates
		return inputPort_->getRelativeOffset(
			inputPort_->getBox().zeroPos().getCenter(), getRack());
    } else if (hoveredInputPort_) {
		return hoveredInputPort_->getRelativeOffset(
			hoveredInputPort_->getBox().zeroPos().getCenter(), getRack());
    } else {
        return getRack()->getMousePos();
    }
}

math::Vec CableWidget::getOutputPortPosInRackCoords() const {
    if (outputPort_) {
        // Return center of the output port in Rack coordinates
        return outputPort_->getRelativeOffset(
            outputPort_->getBox().zeroPos().getCenter(), getRack());
    } else if (hoveredOutputPort_) {
        return hoveredOutputPort_->getRelativeOffset(
            hoveredOutputPort_->getBox().zeroPos().getCenter(), getRack());
    } else {
        return getRack()->getMousePos();
    }
}

void CableWidget::mergeJson(json_t* rootJ) {
	std::string s = color::toHexString(color_);
	json_object_set_new(rootJ, "color", json_string(s.c_str()));

    json_object_set_new(rootJ, "tensionRandomValue",
                        json_real(tensionRandomValue_));
}

void CableWidget::fromJson(json_t* rootJ) {
    json_t* colorJ = json_object_get(rootJ, "color");
    if (colorJ && json_is_string(colorJ)) {
        color_ = color::fromHexString(json_string_value(colorJ));
    } else {
        // In <v0.6.0, cables used JSON objects instead of hex strings. Just
        // ignore them if so and use the existing cable color. In <=v1, cable
        // colors were not serialized.
        color_ = getRack()->getNextCableColor();
    }

    json_t* tensionRandomJ = json_object_get(rootJ, "tensionRandomValue");
    if (tensionRandomJ && json_is_number(tensionRandomJ) &&
        json_number_value(tensionRandomJ) != 0.0) {
        tensionRandomValue_ = json_number_value(tensionRandomJ);
    } else {
        tensionRandomValue_ = generateTensionRandomValue();
    }
}

math::Vec CableWidget::getSlumpVertexInRackCoords() const {
    // Use the output and input port positions for calculating slumpVertex position
    math::Vec pos0InRackCoords = getOutputPortPosInRackCoords();
    math::Vec pos2InRackCoords = getInputPortPosInRackCoords();

    // The x position of the slumpVertex position is simply the average of the two
    // port x positions.
    float xAverage = (pos0InRackCoords.getX() + pos2InRackCoords.getX()) / 2;

    // Lower average point as distance increases.
    // Originally droopage was 150 but there is no good reason for the cables to
    // hang so low.
    double droopage = 50.0;

    // Determine the cabke tension value to use. It is the 0.0-1.0 tension
    // setting for the application plus cableTensionRandomFactor *
    // tensionRandomValue_ where cableTensionRandomFactor is between 0.0 and 0.3
    // and tensionRandomValue_ is a random value between -1.0 and 1.
    float cableTensionWithRandom = math::clamp(
        settings::cableTension +
            settings::cableTensionRandomFactor * tensionRandomValue_,
        0.0f, 1.0f);

    // The y value of the slumpVertex position is the average of the two port y
    // positions, plus an amount based on the distance between the two ports
    // and the cable tension setting.
    float distanceBtwnPorts = pos0InRackCoords.minus(pos2InRackCoords).norm();
    float yAverage = (pos0InRackCoords.getY() + pos2InRackCoords.getY()) / 2;
    float ySlumpPos = yAverage + (1.0 - cableTensionWithRandom) *
                                     (droopage + 1.0 * distanceBtwnPorts);

    // Return the calculated slumpVertex position
    math::Vec slumpPos(xAverage, ySlumpPos);
    return slumpPos;
}

/**
 * @brief Get the slumpVertex Y pos given port locs and minimum y value for
 * curve
 *
 * Definitions: P0 is first port positiion, P1 is position of the vertex point,
 * and P2 is the second port position. y1 is simply P1.y. y_min is the bottom of
 * the Bezier curve.
 *
 * Goal is to be able to have the Bezier curve represeenting the cable not go
 * beyond the bottom of the screen. This is complicated by the curve being
 * specified by the port locs P0 and P2, and also the vertex P1. But need to use
 * the Bezier quadratic equation B(t) = (1-t)^2 * P0 + 2*t*(1-t)*P1 + t^2 * P2.
 * Then can determine P1.y where the curve (not the vertex) is at bottom of
 * scene.
 *
 * Once have P1.y for making the curve be at bottom of scene, can see if this
 * value is greater or less than the original P1.y. If the new value is smaller
 * than the original P1.y then the Bezier curve for the cable is above the
 * scene bottom and can be used. But if new value is greater than original P1.y
 * then the curve would be below the scene bottom and instead the new P1.y
 * value should be used.
 *
 * The derivation of the formula for calculating y1 is quite complicated so used
 * Gemini AI and went over the results. Indeed seem correct. The equation is:
 *
 *   y1 = y_min +- sqrt((y_min - y0)(y_min - y2))
 *
 * But if (y_min - y0)(y_min - y2) < 0 then cannot have a minimum at y_min so
 * returns simply the average of y0 and y2.
 *
 * Note: all positions must be provided in the same coordinate frame. The Rack
 * coordinate frame is probably best since most locations are already relative
 * to Rack coordinates.
 *
 * @param p0 position of first port, which is P0 for defining Bezier curve
 * @param p2 position of second port, which is P2 for defining Bezier curve
 * @param y_min the bottom location of the Bezier curve, which should be set to
 * bottom of scene
 *
 * @return float y1, the value P1.y needs to be for curve to be at bottom of
 * scene
 */
static float getSlumpVertexYGivenPortLocsAndCurveMinY(const math::Vec& p0,
                                                      const math::Vec& p2,
                                                      float y_min) {
    float y0 = p0.getY();
    float y2 = p2.getY();

    if ((y_min - y0) * (y_min - y2) >= 0) {
        // Can have curve minimum at y_min. Return calculated value.
        float y1 = y_min + sqrt((y_min - y0) * (y_min - y2));
        return y1;
    } else {
        // Cannot have curve minimum at y_min since ports are on opposite
        // sides of y_min. Just return average of the two port y positions.
        float y1 = (y0 + y2) / 2;
        return y1;
    }
}

math::Vec CableWidget::getSlumpVertexForCableAtScreenBottomInRackCoords() const {
    math::Vec bottomOfScreenInRackCoords = getRack()->getScenePosInLocalCoords(
        math::Vec(0, getScene()->getHeight()));

    math::Vec p0InRackCoords = getOutputPortPosInRackCoords();
    math::Vec p2InRackCoords = getInputPortPosInRackCoords();

    // Determine the p1 slumpVertex Y position for the bottom of the
    // cable curve to be at the bottom of the screen. Note: if the math
    // was correct then should subtract half CABLE_WIDTH since the cable
    // is drawn centered on the curve. But in practice it looks better
    // to just use CABLE_WIDTH.
    float slumpVertexYInRackCoords = getSlumpVertexYGivenPortLocsAndCurveMinY(
        p0InRackCoords, p2InRackCoords,
        bottomOfScreenInRackCoords.getY() - CABLE_WIDTH);

    // Determine the p1 slumpVertex X position, which is simply the average
    // of the two port X positions
    float slumpVertexXInRackCoords =
        (p0InRackCoords.getX() + p2InRackCoords.getX()) / 2;

    // Return the p1 slumpVertex position for the cable curve to be at
    // the bottom of the screen
    return math::Vec(slumpVertexXInRackCoords, slumpVertexYInRackCoords);
}

void CableWidget::step() {
    cableOutputPlugInRackCoords_ = getOutputPortPosInRackCoords();
    cableInputPlugInRackCoords_ = getInputPortPosInRackCoords();
    slumpVertexInRackCoords_ = getSlumpVertexInRackCoords();

    // Determine the proper slumpVertex position to keep cable visible within
    // scene bounds. Adjust the slumpVertex y position if needed

    // Get p1 slump vertex based just on cable tension. The bottom of
    // the cable might be below the scene bottom.
    math::Vec originalSlumpVertexInRackCoords = slumpVertexInRackCoords_;

    // Determine if should possibly adjust slump vertex to keep cable
    // within screen bounds. Only need to check whether to adjust is if slump
    // vertex is below both ports.
    if (originalSlumpVertexInRackCoords.getY() >
        std::max(cableOutputPlugInRackCoords_.getY(),
                 cableInputPlugInRackCoords_.getY())) {
      // Determine p1 slump vertex such that bottom of cable is right at
      // bottom of scene and therefore fully visible
      math::Vec slumpVertexForCurveAtScreenBottomInRackCoords =
          getSlumpVertexForCableAtScreenBottomInRackCoords();

      // If the slump vertex Y is below where it needs to be to keep cable
      // within screen bounds, then adjust slump vertex upward
      if (originalSlumpVertexInRackCoords.getY() >
          slumpVertexForCurveAtScreenBottomInRackCoords.getY()) {
        // Current slumpVertex is below where it needs to be to keep cable
        // within screen bounds, so adjust it upward
        slumpVertexInRackCoords_ =
            slumpVertexForCurveAtScreenBottomInRackCoords;
      }
    }

    // The endpoints of cable shouldn't go all the way to center of plug.
    // This way the cable won't be drawn over the plugs.
    float outputPlugDistance = 17.f;
    cableOutputEndInRackCoords_ = cableOutputPlugInRackCoords_.plus(
        slumpVertexInRackCoords_.minus(cableOutputPlugInRackCoords_)
            .normalize()
            .mult(outputPlugDistance));

    float inputPlugDistance = 16.f;
    cableInputEndInRackCoords_ = cableInputPlugInRackCoords_.plus(
        slumpVertexInRackCoords_.minus(cableInputPlugInRackCoords_)
            .normalize()
            .mult(inputPlugDistance));

    // Setup opaqueness color for the plugs
    NVGcolor colorFullyOpaque = color_;
    colorFullyOpaque.a = 1.f;

    // Setup drawing of output plug
    outputPlug_->setPos(cableOutputPlugInRackCoords_);
    bool outputPortOnTop =
        outputPort_ && (getRack()->getTopPlug(outputPort_) == outputPlug_);
    outputPlug_->setTop(outputPortOnTop);
    outputPlug_->setAngle(
        slumpVertexInRackCoords_.minus(cableOutputPlugInRackCoords_).arg());
    outputPlug_->setColor(colorFullyOpaque);

    // Setup drawing of input plug
    inputPlug_->setPos(cableInputPlugInRackCoords_);
    bool inputPortOnTop =
        inputPort_ && (getRack()->getTopPlug(inputPort_) == inputPlug_);
    inputPlug_->setTop(inputPortOnTop);
    inputPlug_->setAngle(
        slumpVertexInRackCoords_.minus(cableInputPlugInRackCoords_).arg());
    inputPlug_->setColor(colorFullyOpaque);

    // Continue with normal step processing
    Widget::step();
}

void CableWidget::draw(const DrawArgs& args) {
    CableWidget::drawLayer(args, 0);
}

void CableWidget::drawLayer(const DrawArgs& args, int layer) {
    // Determine opacity for drawing cable and shadow
    float opacity = settings::cableOpacity;
    bool thick = false;

    // Determine opacity to use for drawing the cable
    if (isComplete()) {
        // Cable connected on both ends to port, so determine desired opacity of
        // cable accordingly
        engine::Output* output = &cable_->outputModule->outputs[cable_->outputId];

        // Increase thickness if output port is polyphonic
        if (output->isPolyphonic()) {
            thick = true;
        }

        // Draw opaque if mouse is hovering over a connected port
        Widget* hoveredWidget = getEvent()->getHoveredWidget();
        if (outputPort_ == hoveredWidget || inputPort_ == hoveredWidget) {
            opacity = 1.0;
        }
        // Draw translucent cable if not active (i.e. 0 channels)
        else if (output->getChannels() == 0) {
            opacity *= 0.5;
        }
    } else {
        // Draw opaque since the cable is incomplete
        opacity = 1.0;
    }

    if (opacity <= 0.0) return;
    nvgAlpha(args.vg, std::pow(opacity, 1.5));

    // Set how thick the cable should be drawn
    float thickness = thick ? THICK_CABLE_WIDTH : CABLE_WIDTH;

    // Best line cap seems to be rounded
    nvgLineCap(args.vg, NVG_ROUND);

    // Avoids glitches when cable is bent
    nvgLineJoin(args.vg, NVG_ROUND);

    if (layer == -1) {
        // layer is -1 so draw the cable shadows

        // Setup to draw cable shadow using a slumpVertex point below the cable
        float shadowDeltaPxls = 15.f;
        math::Vec shadowSlumpVertexInRackCoords =
            slumpVertexInRackCoords_.plus(math::Vec(0, shadowDeltaPxls));

        // Draw cable shadow using bezier curve with shadowSlumpVertexInRackCoords point
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, VEC_ARGS(cableOutputEndInRackCoords_));
        nvgQuadTo(args.vg, VEC_ARGS(shadowSlumpVertexInRackCoords),
                  VEC_ARGS(cableInputEndInRackCoords_));
        NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.12);
        nvgStrokeColor(args.vg, shadowColor);
        nvgStrokeWidth(args.vg, thickness - 1.0);
        nvgStroke(args.vg);

        // And now draw shadow but in white so that it shows up
        // on top of black panels as well
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, VEC_ARGS(cableOutputEndInRackCoords_));
        nvgQuadTo(args.vg, VEC_ARGS(shadowSlumpVertexInRackCoords),
                  VEC_ARGS(cableInputEndInRackCoords_));
        NVGcolor shadowColorForDarkPanel = nvgRGBAf(1.0, 1.0, 1.0, 0.12);
        nvgStrokeColor(args.vg, shadowColorForDarkPanel);
        nvgStrokeWidth(args.vg, thickness - 1.0);
        nvgStroke(args.vg);
    } else if (layer == 0) {
        // layer is 0 so draw the cable itself

        // Setup points to draw cable
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, VEC_ARGS(cableOutputEndInRackCoords_));
        nvgQuadTo(args.vg, VEC_ARGS(slumpVertexInRackCoords_),
                  VEC_ARGS(cableInputEndInRackCoords_));

        // Do the stroke for the cable outline. The outline has slightly
        // lower opacity than the cable itself.
        nvgStrokeColor(args.vg, color::mult(color_, 0.8));
        nvgStrokeWidth(args.vg, thickness);
        nvgStroke(args.vg);

        // Do the stroke for the inner part of the cable. This is slightly
        // thinner than the outline and has slightly higher opacity.
        nvgStrokeColor(args.vg, color::mult(color_, 0.95));
        nvgStrokeWidth(args.vg, thickness - 1.0);
        nvgStroke(args.vg);
    }

    // Draw children widgets, such as the plugs
    Widget::drawLayer(args, layer);
}

engine::Cable* CableWidget::releaseCable() {
    engine::Cable* cable = this->cable_;
    this->cable_ = NULL;
    cableInternals_->cableId = -1;
    return cable;
}

void CableWidget::onAdd(const AddEvent& e) {
	Widget* plugContainer = getRack()->getPlugContainer();
	plugContainer->addChild(outputPlug_);
	plugContainer->addChild(inputPlug_);
	Widget::onAdd(e);
}


void CableWidget::onRemove(const RemoveEvent& e) {
	Widget* plugContainer = getRack()->getPlugContainer();
	plugContainer->removeChild(outputPlug_);
	plugContainer->removeChild(inputPlug_);
	Widget::onRemove(e);
}


} // namespace app
} // namespace rack
