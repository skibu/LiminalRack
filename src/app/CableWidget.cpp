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
	cableInternals = new CableInternal;
	color = color::BLACK_TRANSPARENT;

	outputPlug = new PlugWidget(this, engine::Port::OUTPUT);
	inputPlug = new PlugWidget(this, engine::Port::INPUT);
}


CableWidget::~CableWidget() {
	delete outputPlug;
	delete inputPlug;

	setCable(NULL);
	delete cableInternals;
}


bool CableWidget::isComplete() {
	return outputPort && inputPort;
}


void CableWidget::updateCable() {
	// Clean up existing cable if it exists
	if (cable) {
		getEngine()->removeCable(cable);
		delete cable;
		cable = NULL;
	}

	// If either input or output port not set then cannot create cable
	if (!inputPort || !outputPort) return;

	// Both ports are set, create a new cable
	cable = new engine::Cable;
	cable->id = cableInternals->cableId;
	cable->inputModule = inputPort->module;
	cable->inputId = inputPort->portId;
	cable->outputModule = outputPort->module;
	cable->outputId = outputPort->portId;
	getEngine()->addCable(cable);
	cableInternals->cableId = cable->id;

	// Make sure cable color is correct. It was originally set based
	// on the first port, but that port might not have definitively
	// determined the color, perhaps because name of the port had
	// insufficient information.
    color = CableColorMatcher::getCableColor(inputPort->getPortInfo(),
											outputPort->getPortInfo());
}

void CableWidget::setCable(engine::Cable* cable) {
	if (this->cable) {
		getEngine()->removeCable(this->cable);
		delete this->cable;
		this->cable = NULL;
		cableInternals->cableId = -1;
	}
	if (cable) {
		app::ModuleWidget* outputMw = getRack()->getModule(cable->outputModule->id);
		if (!outputMw)
			throw Exception("Cable cannot find output ModuleWidget %lld", (long long) cable->outputModule->id);
		outputPort = outputMw->getOutput(cable->outputId);
		if (!outputPort)
			throw Exception("Cable cannot find output port %d", cable->outputId);

		app::ModuleWidget* inputMw = getRack()->getModule(cable->inputModule->id);
		if (!inputMw)
			throw Exception("Cable cannot find input ModuleWidget %lld", (long long) cable->inputModule->id);
		inputPort = inputMw->getInput(cable->inputId);
		if (!inputPort)
			throw Exception("Cable cannot find input port %d", cable->inputId);

		this->cable = cable;
		cableInternals->cableId = cable->id;
	}
	else {
		outputPort = NULL;
		inputPort = NULL;
	}
}


engine::Cable* CableWidget::getCable() {
	return cable;
}

math::Vec CableWidget::getInputPos() {
    if (inputPort) {
		return inputPort->getRelativeOffset(
			inputPort->getBox().zeroPos().getCenter(), getRack());
    } else if (hoveredInputPort) {
		return hoveredInputPort->getRelativeOffset(
			hoveredInputPort->getBox().zeroPos().getCenter(), getRack());
    } else {
        return getRack()->getMousePos();
    }
}

math::Vec CableWidget::getOutputPos() {
	if (outputPort) {
		return outputPort->getRelativeOffset(outputPort->getBox().zeroPos().getCenter(), getRack());
	}
	else if (hoveredOutputPort) {
		return hoveredOutputPort->getRelativeOffset(hoveredOutputPort->getBox().zeroPos().getCenter(), getRack());
	}
	else {
		return getRack()->getMousePos();
	}
}


void CableWidget::mergeJson(json_t* rootJ) {
	std::string s = color::toHexString(color);
	json_object_set_new(rootJ, "color", json_string(s.c_str()));
}


void CableWidget::fromJson(json_t* rootJ) {
	json_t* colorJ = json_object_get(rootJ, "color");
	if (colorJ && json_is_string(colorJ)) {
		color = color::fromHexString(json_string_value(colorJ));
	}
	else {
		// In <v0.6.0, cables used JSON objects instead of hex strings. Just ignore them if so and use the existing cable color.
		// In <=v1, cable colors were not serialized.
		color = getRack()->getNextCableColor();
	}
}


static math::Vec getSlumpPos(math::Vec pos1, math::Vec pos2) {
	float dist = pos1.minus(pos2).norm();
	math::Vec avg = pos1.plus(pos2).div(2);
	// Lower average point as distance increases. 
	// Originally droopage was 150 but there is no good reason for
	// the cables to hang so low.
	double droopage = 50.0;
	avg.setY(avg.getY() + (1.0 - settings::cableTension) * (droopage + 1.0 * dist));
	return avg;
}

static int c = 0;

void CableWidget::step() {
	math::Vec outputPos = getOutputPos();
	math::Vec inputPos = getInputPos();
	math::Vec slump = getSlumpPos(outputPos, inputPos);

    // FIXME for debugging coordinates : Remove debug output
    if (c++ % 180 == 0) {
        DEBUG(
            "step: outputPos=(%.1f, %.1f) inputPos=(%.1f, "
            "%.1f) slump=(%.1f, %.1f)",
            outputPos.getX(), outputPos.getY(), inputPos.getX(), inputPos.getY(), slump.getX(),
            slump.getY());

        DEBUG("Scene box: x=%f, y=%f w=%f, h=%f", getScene()->getX(),
             getScene()->getY(), getScene()->getWidth(),
             getScene()->getHeight());

        // FIXME for debugging coordinates trying using getAbsoluteOffset() to convert to absolute coords
        math::Vec initialVec = slump;
        math::Vec absoluteVec = getAbsoluteOffset(initialVec);
        DEBUG("--absoluteVecOfSlump=(%f, %f)",  absoluteVec.getX(), absoluteVec.getY());
    }

    NVGcolor colorOpaque = color;
	colorOpaque.a = 1.f;

	// Setup drawing of output plug
	outputPlug->setPos(outputPos);
	bool outputTop = outputPort && (getRack()->getTopPlug(outputPort) == outputPlug);
	outputPlug->setTop(outputTop);
	outputPlug->setAngle(slump.minus(outputPos).arg());
	outputPlug->setColor(colorOpaque);

	// Setup drawing of input plug
	inputPlug->setPos(inputPos);
	bool inputTop = inputPort && (getRack()->getTopPlug(inputPort) == inputPlug);
	inputPlug->setTop(inputTop);
	inputPlug->setAngle(slump.minus(inputPos).arg());
	inputPlug->setColor(colorOpaque);

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
		// Cable connected on both ends to port, so determine desired opacity of cable accordingly 
		engine::Output* output = &cable->outputModule->outputs[cable->outputId];

		// Increase thickness if output port is polyphonic
		if (output->isPolyphonic()) {
			thick = true;
		}

		// Draw opaque if mouse is hovering over a connected port
		Widget* hoveredWidget = getEvent()->hoveredWidget;
		if (outputPort == hoveredWidget || inputPort == hoveredWidget) {
			opacity = 1.0;
		}
		// Draw translucent cable if not active (i.e. 0 channels)
		else if (output->getChannels() == 0) {
			opacity *= 0.5;
		}
	}
	else {
		// Draw opaque since the cable is incomplete
		opacity = 1.0;
	}

	if (opacity <= 0.0)
		return;
	nvgAlpha(args.vg, std::pow(opacity, 1.5));

	// Determine how to draw the cable
	math::Vec outputPos = getOutputPos();
	math::Vec inputPos = getInputPos();

	// Set how thick the cable should be drawn
	float thickness = thick ? 9.0 : 6.0;

	// The endpoints of cable don't go all the way to center of jack.
	// For output jack they should go closer to look somewhat like an arrow.
	math::Vec slump = getSlumpPos(outputPos, inputPos);
	float outputJackDistance = 17.f;
	outputPos = outputPos.plus(slump.minus(outputPos).normalize().mult(outputJackDistance));
	float inputJackDistance = 16.f;
	inputPos = inputPos.plus(slump.minus(inputPos).normalize().mult(inputJackDistance));

	// Best line cap seems to be rounded
	nvgLineCap(args.vg, NVG_ROUND);

	// Avoids glitches when cable is bent
	nvgLineJoin(args.vg, NVG_ROUND);

	if (layer == -1) {
		// Setup to draw cable shadow using a slump point below the cable
		float shadowDeltaPxls = 15.f;
		math::Vec shadowSlump = slump.plus(math::Vec(0, shadowDeltaPxls));

		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, VEC_ARGS(outputPos));
		nvgQuadTo(args.vg, VEC_ARGS(shadowSlump), VEC_ARGS(inputPos));
		NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.12);
		nvgStrokeColor(args.vg, shadowColor);
		nvgStrokeWidth(args.vg, thickness - 1.0);
		nvgStroke(args.vg);

		// And now draw shadow but in white so that it shows up
		// on top of black panels as well
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, VEC_ARGS(outputPos));
		nvgQuadTo(args.vg, VEC_ARGS(shadowSlump), VEC_ARGS(inputPos));
		NVGcolor shadowColorForDarkPanel = nvgRGBAf(1.0, 1.0, 1.0, 0.12);
		nvgStrokeColor(args.vg, shadowColorForDarkPanel);
		nvgStrokeWidth(args.vg, thickness - 1.0);
		nvgStroke(args.vg);

        }
	else if (layer == 0) {
		// Setup to draw cable outline
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, VEC_ARGS(outputPos));
		nvgQuadTo(args.vg, VEC_ARGS(slump), VEC_ARGS(inputPos));
		// nvgStrokePaint(args.vg, nvgLinearGradient(args.vg, VEC_ARGS(outputPos), VEC_ARGS(inputPos), color::mult(color, 0.5), color));
		nvgStrokeColor(args.vg, color::mult(color, 0.8));
		nvgStrokeWidth(args.vg, thickness);
		nvgStroke(args.vg);

		// Draw cable
		nvgStrokeColor(args.vg, color::mult(color, 0.95));
		nvgStrokeWidth(args.vg, thickness - 1.0);
		nvgStroke(args.vg);
	}

	// Draw children widgets
	Widget::drawLayer(args, layer);
}


engine::Cable* CableWidget::releaseCable() {
	engine::Cable* cable = this->cable;
	this->cable = NULL;
	cableInternals->cableId = -1;
	return cable;
}


void CableWidget::onAdd(const AddEvent& e) {
	Widget* plugContainer = getRack()->getPlugContainer();
	plugContainer->addChild(outputPlug);
	plugContainer->addChild(inputPlug);
	Widget::onAdd(e);
}


void CableWidget::onRemove(const RemoveEvent& e) {
	Widget* plugContainer = getRack()->getPlugContainer();
	plugContainer->removeChild(outputPlug);
	plugContainer->removeChild(inputPlug);
	Widget::onRemove(e);
}


} // namespace app
} // namespace rack
