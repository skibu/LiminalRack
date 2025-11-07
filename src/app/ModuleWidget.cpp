#include <thread>
#include <regex>

#include <osdialog.h>

#include <app/ModuleWidget.hpp>
#include <app/Scene.hpp>
#include <engine/Engine.hpp>
#include <plugin/Plugin.hpp>
#include <app/SvgPanel.hpp>
#include <ui/liminal.hpp>
#include <ui/MenuSeparator.hpp>
#include <system.hpp>
#include <asset.hpp>
#include <helpers.hpp>
#include <context.hpp>
#include <settings.hpp>
#include <history.hpp>
#include <string.hpp>
#include <componentlibrary.hpp>


namespace rack {
namespace app {


static const char PRESET_FILTERS[] = "VCV Rack module preset (.vcvm):vcvm";


struct ModuleWidget::Internal {
	/** The module position clicked on to start dragging in the rack.
	*/
	math::Vec dragOffset;

	/** Global rack position the user clicked on.
	*/
	math::Vec dragRackPos;
	bool dragEnabled = true;

	widget::Widget* panel = NULL;
};


ModuleWidget::ModuleWidget() {
	internal_ = new Internal;
	setSize(math::Vec(0, RACK_GRID_HEIGHT));
}

ModuleWidget::~ModuleWidget() {
	clearChildren();
	setModule(NULL);
	delete internal_;
}

plugin::Model* ModuleWidget::getModel() {
	return model;
}

void ModuleWidget::setModel(plugin::Model* model) {
	assert(!this->model);
	this->model = model;
}

engine::Module* ModuleWidget::getModule() {
	return module;
}

void ModuleWidget::setModule(engine::Module* module) {
	if (this->module) {
		getEngine()->removeModule(this->module);
		delete this->module;
		this->module = NULL;
	}
	this->module = module;
}

widget::Widget* ModuleWidget::getPanel() {
	return internal_->panel;
}

void ModuleWidget::setPanel(widget::Widget* panel) {
	// Remove existing panel
	if (internal_->panel) {
		removeChild(internal_->panel);
		delete internal_->panel;
		internal_->panel = NULL;
	}

	if (panel) {
		addChildBottom(panel);
		internal_->panel = panel;
		setWidth(std::round(panel->getWidth() / RACK_GRID_WIDTH) * RACK_GRID_WIDTH);
		// If width is zero, set it to 12HP for sanity
		if (getWidth() == 0.0)
			setWidth(12 * RACK_GRID_WIDTH);
	}
}

void ModuleWidget::setPanel(std::shared_ptr<window::Svg> svg) {
	// Create SvgPanel
	SvgPanel* panel = new SvgPanel;
	panel->setBackground(svg);
	setPanel(panel);
}

void ModuleWidget::addParam(ParamWidget* param) {
	addChild(param);
}

void ModuleWidget::addInput(PortWidget* input) {
	// Check that the port is an input
	assert(input->type == engine::Port::INPUT);
	// Check that the port doesn't have a duplicate ID
	PortWidget* input2 = getInput(input->portId);
	assert(!input2);
	// Add port
	addChild(input);
}

void ModuleWidget::addOutput(PortWidget* output) {
	// Check that the port is an output
	assert(output->type == engine::Port::OUTPUT);
	// Check that the port doesn't have a duplicate ID
	PortWidget* output2 = getOutput(output->portId);
	assert(!output2);
	// Add port
	addChild(output);
}

template <class T, typename F>
T* getFirstDescendantOfTypeWithCondition(widget::Widget* w, F f) {
	T* t = dynamic_cast<T*>(w);
	if (t && f(t))
		return t;

	for (widget::Widget* child : w->getChildren()) {
		T* foundT = getFirstDescendantOfTypeWithCondition<T>(child, f);
		if (foundT)
			return foundT;
	}
	return NULL;
}

ParamWidget* ModuleWidget::getParam(int paramId) {
	return getFirstDescendantOfTypeWithCondition<ParamWidget>(this, [&](ParamWidget* pw) -> bool {
		return pw->paramId == paramId;
	});
}

PortWidget* ModuleWidget::getInput(int portId) {
	return getFirstDescendantOfTypeWithCondition<PortWidget>(this, [&](PortWidget* pw) -> bool {
		return pw->type == engine::Port::INPUT && pw->portId == portId;
	});
}

PortWidget* ModuleWidget::getOutput(int portId) {
	return getFirstDescendantOfTypeWithCondition<PortWidget>(this, [&](PortWidget* pw) -> bool {
		return pw->type == engine::Port::OUTPUT && pw->portId == portId;
	});
}

template <class T, typename F>
void doIfTypeRecursive(widget::Widget* w, F f) {
	T* t = dynamic_cast<T*>(w);
	if (t)
		f(t);

	for (widget::Widget* child : w->getChildren()) {
		doIfTypeRecursive<T>(child, f);
	}
}

std::vector<ParamWidget*> ModuleWidget::getParams() {
	std::vector<ParamWidget*> pws;
	doIfTypeRecursive<ParamWidget>(this, [&](ParamWidget* pw) {
		pws.push_back(pw);
	});
	return pws;
}

std::vector<PortWidget*> ModuleWidget::getPorts() {
	std::vector<PortWidget*> pws;
	doIfTypeRecursive<PortWidget>(this, [&](PortWidget* pw) {
		pws.push_back(pw);
	});
	return pws;
}

std::vector<PortWidget*> ModuleWidget::getInputs() {
	std::vector<PortWidget*> pws;
	doIfTypeRecursive<PortWidget>(this, [&](PortWidget* pw) {
		if (pw->type == engine::Port::INPUT)
			pws.push_back(pw);
	});
	return pws;
}

std::vector<PortWidget*> ModuleWidget::getOutputs() {
	std::vector<PortWidget*> pws;
	doIfTypeRecursive<PortWidget>(this, [&](PortWidget* pw) {
		if (pw->type == engine::Port::OUTPUT)
			pws.push_back(pw);
	});
	return pws;
}

void ModuleWidget::draw(const DrawArgs& args) {
    nvgScissor(args.vg, RECT_ARGS(args.clipBox));

    if (module && module->isBypassed()) {
        nvgAlpha(args.vg, 0.33);
    }

    Widget::draw(args);

    // Meter
    if (module && settings::cpuMeter) {
        float sampleRate = getEngine()->getSampleRate();
        const float* meterBuffer = module->meterBuffer();
        int meterLength = module->meterLength();
        int meterIndex = module->meterIndex();

        // // Text background
        // nvgBeginPath(args.vg);
        // nvgRect(args.vg, 0.0, box.size.y - infoHeight, box.size.x,
        // infoHeight); nvgFillColor(args.vg, nvgRGBAf(0, 0, 0, 0.75));
        // nvgFill(args.vg);

        // Draw time plot
        const float plotHeight = getHeight() - rack::settings::bndWidgetHeight;
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0.0, plotHeight);
        math::Vec p1;
        for (int i = 0; i < meterLength; i++) {
            int index = math::eucMod(meterIndex + i + 1, meterLength);
            float meter =
                math::clamp(meterBuffer[index] * sampleRate, 0.f, 1.f);
            meter = std::max(0.f, meter);
            math::Vec p;
            p.set((float)i / (meterLength - 1) * getWidth(),
                  (1.f - meter) * plotHeight);
            if (i == 0) {
                nvgLineTo(args.vg, VEC_ARGS(p));
            } else {
                math::Vec p2 = p;
                p2.setX(p2.getX() - 0.5f / (meterLength - 1) * getWidth());
                nvgBezierTo(args.vg, VEC_ARGS(p1), VEC_ARGS(p2), VEC_ARGS(p));
            }
            p1 = p;
            p1.setX(p1.getX() + 0.5f / (meterLength - 1) * getWidth());
        }
        nvgLineTo(args.vg, getWidth(), plotHeight);
        nvgClosePath(args.vg);
        NVGcolor color = componentlibrary::SCHEME_ORANGE;
        nvgFillColor(args.vg, color::alpha(color, 0.75));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 2.0);
        nvgStrokeColor(args.vg, color);
        nvgStroke(args.vg);

        // Text background
        bndMenuBackground(args.vg, 0.0, plotHeight, getWidth(),
                          rack::settings::bndWidgetHeight, BND_CORNER_ALL);

        // Text
        float percent = meterBuffer[meterIndex] * sampleRate * 100.f;
        // float microseconds = meterBuffer[meterIndex] * 1e6f;
        std::string meterText = string::f("%.1f", percent);
        // Only append "%" if wider than 2 HP
        if (getWidth() > RACK_GRID_WIDTH * 2) meterText += "%";
        math::Vec pt(
            getWidth() - bndLabelWidth(args.vg, -1, meterText.c_str()) + 3,
            plotHeight + 0.5);
        bndMenuLabel(args.vg, VEC_ARGS(pt), INFINITY,
                     rack::settings::bndWidgetHeight, -1, meterText.c_str());
    }

    // Selection
    if (getRack()->isSelected(this)) {
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0.0, 0.0, VEC_ARGS(getSize()));
        nvgFillColor(args.vg, nvgRGBAf(1, 0, 0, 0.25));
        nvgFill(args.vg);
        nvgStrokeWidth(args.vg, 2.0);
        nvgStrokeColor(args.vg, nvgRGBAf(1, 0, 0, 0.5));
        nvgStroke(args.vg);
    }

    nvgResetScissor(args.vg);
}

void ModuleWidget::drawLayer(const DrawArgs& args, int layer) {
	if (layer == -1) {
		nvgBeginPath(args.vg);
		float r = 20; // Blur radius
		float c = 20; // Corner radius
		math::Rect shadowBox = getBox().zeroPos().grow(math::Vec(10, -30));
		math::Rect shadowOutsideBox = shadowBox.grow(math::Vec(r, r));
		nvgRect(args.vg, RECT_ARGS(shadowOutsideBox));
		NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.2);
		NVGcolor transparentColor = nvgRGBAf(0, 0, 0, 0);
		nvgFillPaint(args.vg, nvgBoxGradient(args.vg, RECT_ARGS(shadowBox), c, r, shadowColor, transparentColor));
		nvgFill(args.vg);
	}
	else {
		Widget::drawLayer(args, layer);
	}
}

void ModuleWidget::onHover(const HoverEvent& e) {
	if (getRack()->isSelected(this)) {
		e.consume(this);
	}

	OpaqueWidget::onHover(e);
}

void ModuleWidget::onHoverKey(const HoverKeyEvent& e) {
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		if (e.isKeyCommand(GLFW_KEY_C, RACK_MOD_CTRL)) {
			copyClipboard();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_V, RACK_MOD_CTRL)) {
			if (pasteClipboardAction()) {
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_D, RACK_MOD_CTRL)) {
			cloneAction(false);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_D, RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			cloneAction(true);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_I, RACK_MOD_CTRL)) {
			resetAction();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_R, RACK_MOD_CTRL)) {
			randomizeAction();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_U, RACK_MOD_CTRL)) {
			disconnectAction();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_E, RACK_MOD_CTRL)) {
			bypassAction(!module->isBypassed());
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_DELETE) || e.isKeyCommand(GLFW_KEY_BACKSPACE)) {
			// Deletes `this`
			removeAction();
			e.consume(NULL);
			return;
		}
		if (e.isKeyCommand(GLFW_KEY_F1, RACK_MOD_CTRL)) {
			std::string manualUrl = model->getManualUrl();
			if (!manualUrl.empty())
				system::openBrowser(manualUrl);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_F4, RACK_MOD_CTRL)) {
			getScene()->getRackScroll()->zoomToBound(getBox());
			e.consume(this);
		}
	}

	if (e.isConsumed())
		return;
	OpaqueWidget::onHoverKey(e);
}

void ModuleWidget::onButton(const ButtonEvent& e) {
	bool selected = getRack()->isSelected(this);

	if (selected) {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (e.action == GLFW_PRESS) {
				// Open selection context menu on right-click
				ui::Menu* menu = createMenu();
				getRack()->appendSelectionContextMenu(menu);
			}
			e.consume(this);
		}

		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (e.action == GLFW_PRESS) {
				// Toggle selection on Shift-click
				if ((e.mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
					getRack()->select(this, false);
					e.consume(NULL);
					return;
				}

				// If module positions are locked, don't consume left-click
				if (settings::lockModules) {
					e.consume(NULL);
					return;
				}

				internal_->dragOffset = e.pos;
			}

			e.consume(this);
		}

		return;
	}

	// Dispatch event to children
	Widget::onButton(e);
	e.stopPropagating();
	if (e.isConsumed())
		return;

	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		if (e.action == GLFW_PRESS) {
			// Toggle selection on Shift-click
			if ((e.mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
				getRack()->select(this, true);
				e.consume(NULL);
				return;
			}

			// If module positions are locked, don't consume left-click
			if (settings::lockModules) {
				e.consume(NULL);
				return;
			}

			internal_->dragOffset = e.pos;
		}
		e.consume(this);
	}

	// Open context menu on right-click
	if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
		createContextMenu();
		e.consume(this);
	}
}

void ModuleWidget::onDragStart(const DragStartEvent& e) {
	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		// HACK Disable FramebufferWidget redrawing subpixels while dragging
		getWindow()->fbDirtyOnSubpixelChange() = false;

		// Clear dragRack so dragging in not enabled until mouse is moved a bit.
		internal_->dragRackPos = math::Vec(NAN, NAN);

		// Prepare initial position of modules for history.
		getRack()->updateModuleOldPositions();
	}
}

void ModuleWidget::onDragEnd(const DragEndEvent& e) {
	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		getWindow()->fbDirtyOnSubpixelChange() = true;

		// The next time the module is dragged, it should always move immediately
		internal_->dragEnabled = true;

		history::ComplexAction* h = getRack()->getModuleDragAction();
		if (!h->isEmpty())
			getHistory()->push(h);
		else
			delete h;
	}
}

void ModuleWidget::onDragMove(const DragMoveEvent& e) {
	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		math::Vec mousePos = getRack()->getMousePos();

		if (!internal_->dragEnabled) {
			// Set dragRackPos on the first time after dragging
			if (!internal_->dragRackPos.isFinite())
				internal_->dragRackPos = mousePos;
			// Check if the mouse has moved enough to start dragging the module.
			const float minDist = RACK_GRID_WIDTH;
			if (internal_->dragRackPos.minus(mousePos).square() >= std::pow(minDist, 2))
				internal_->dragEnabled = true;
		}

		// Move module
		if (internal_->dragEnabled) {
			// Round y coordinate to nearest rack height
			math::Vec pos = mousePos;
			pos.set(pos.getX() - internal_->dragOffset.getX(), pos.getY() - RACK_GRID_HEIGHT / 2);

			if (getRack()->isSelected(this)) {
				pos = (pos / RACK_GRID_SIZE).round() * RACK_GRID_SIZE;
				math::Vec delta = pos.minus(getPos());
				getRack()->setSelectionPosNearest(delta);
			}
			else {
				if (settings::squeezeModules) {
					getRack()->setModulePosSqueeze(this, pos);
				}
				else {
					if ((getWindow()->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL)
						getRack()->setModulePosForce(this, pos);
					else
						getRack()->setModulePosNearest(this, pos);
				}
			}
		}
	}
}

void ModuleWidget::onDragHover(const DragHoverEvent& e) {
	if (getRack()->isSelected(this)) {
		e.consume(this);
	}

	OpaqueWidget::onDragHover(e);
}

json_t* ModuleWidget::toJson() {
	json_t* moduleJ = getEngine()->moduleToJson(module);
	return moduleJ;
}

void ModuleWidget::fromJson(json_t* moduleJ) {
	getEngine()->moduleFromJson(module, moduleJ);
}

bool ModuleWidget::pasteJsonAction(json_t* moduleJ) {
	engine::Module::jsonStripIds(moduleJ);

	json_t* oldModuleJ = toJson();
	DEFER({json_decref(oldModuleJ);});

	try {
		fromJson(moduleJ);
	}
	catch (Exception& e) {
		WARN("%s", e.what());
		return false;
	}

	// history::ModuleChange
	history::ModuleChange* h = new history::ModuleChange;
	h->name = string::translate("ModuleWidget.history.pastePreset");
	h->moduleId = module->id;
	json_incref(oldModuleJ);
	h->oldModuleJ = oldModuleJ;
	json_incref(moduleJ);
	h->newModuleJ = moduleJ;
	getHistory()->push(h);
	return true;
}

void ModuleWidget::copyClipboard() {
	json_t* moduleJ = toJson();
	engine::Module::jsonStripIds(moduleJ);

	DEFER({json_decref(moduleJ);});
	char* json = json_dumps(moduleJ, JSON_INDENT(2));
	DEFER({std::free(json);});
	glfwSetClipboardString(getWindow()->getGLFWwindow(), json);
}

bool ModuleWidget::pasteClipboardAction() {
	const char* json = glfwGetClipboardString(getWindow()->getGLFWwindow());
	if (!json) {
		WARN("Could not get text from clipboard.");
		return false;
	}

	json_error_t error;
	json_t* moduleJ = json_loads(json, 0, &error);
	if (!moduleJ) {
		WARN("JSON parsing error at %s %d:%d %s", error.source, error.line, error.column, error.text);
		return false;
	}
	DEFER({json_decref(moduleJ);});

	return pasteJsonAction(moduleJ);
}

void ModuleWidget::load(std::string filename) {
	FILE* file = std::fopen(filename.c_str(), "r");
	if (!file)
		throw Exception("Could not load patch file %s", filename.c_str());
	DEFER({std::fclose(file);});

	INFO("Loading preset %s", filename.c_str());

	json_error_t error;
	json_t* moduleJ = json_loadf(file, 0, &error);
	if (!moduleJ)
		throw Exception("File is not a valid patch file. JSON parsing error at %s %d:%d %s", error.source, error.line, error.column, error.text);
	DEFER({json_decref(moduleJ);});

	engine::Module::jsonStripIds(moduleJ);
	fromJson(moduleJ);
}

void ModuleWidget::loadAction(std::string filename) {
	// history::ModuleChange
	history::ModuleChange* h = new history::ModuleChange;
	h->name = string::translate("ModuleWidget.history.loadPreset");
	h->moduleId = module->id;
	h->oldModuleJ = toJson();

	try {
		load(filename);
	}
	catch (Exception& e) {
		delete h;
		throw;
	}

	// TODO We can use `moduleJ` here instead to save a toJson() call.
	h->newModuleJ = toJson();
	getHistory()->push(h);
}

void ModuleWidget::loadTemplate() {
	std::string templatePath = system::join(model->getUserPresetDirectory(), "template.vcvm");
	try {
		load(templatePath);
	}
	catch (Exception& e) {
		// Do nothing
	}
}

void ModuleWidget::loadDialog() {
	std::string presetDir = model->getUserPresetDirectory();
	system::createDirectories(presetDir);

	// Delete directories if empty
	DEFER({
		try {
			system::remove(presetDir);
			system::remove(system::getDirectory(presetDir));
		}
		catch (Exception& e) {
			// Ignore exceptions if directory cannot be removed.
		}
	});

	osdialog_filters* filters = osdialog_filters_parse(PRESET_FILTERS);
	DEFER({osdialog_filters_free(filters);});

	char* pathC = osdialog_file(OSDIALOG_OPEN, presetDir.c_str(), NULL, filters);
	if (!pathC) {
		// No path selected
		return;
	}
	DEFER({std::free(pathC);});

	try {
		loadAction(pathC);
	}
	catch (Exception& e) {
		osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, e.what());
	}
}

void ModuleWidget::save(std::string filename) {
	INFO("Saving preset %s", filename.c_str());

	json_t* moduleJ = toJson();
	assert(moduleJ);
	DEFER({json_decref(moduleJ);});

	engine::Module::jsonStripIds(moduleJ);

	FILE* file = std::fopen(filename.c_str(), "w");
	if (!file) {
		std::string message = string::f(string::translate("ModuleWidget.savePresetFailed"), filename);
		osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
		return;
	}
	DEFER({std::fclose(file);});

	json_dumpf(moduleJ, file, JSON_INDENT(2));
}

void ModuleWidget::saveTemplate() {
	std::string presetDir = model->getUserPresetDirectory();
	system::createDirectories(presetDir);
	std::string templatePath = system::join(presetDir, "template.vcvm");
	save(templatePath);
}

void ModuleWidget::saveTemplateDialog() {
	if (hasTemplate()) {
		std::string message = string::f(string::translate("ModuleWidget.overwriteTemplate"), model->getFullName());
		if (!osdialog_message(OSDIALOG_INFO, OSDIALOG_OK_CANCEL, message.c_str()))
			return;
	}
	saveTemplate();
}

bool ModuleWidget::hasTemplate() {
	std::string presetDir = model->getUserPresetDirectory();
	std::string templatePath = system::join(presetDir, "template.vcvm");
	return system::exists(templatePath);;
}

void ModuleWidget::clearTemplate() {
	std::string presetDir = model->getUserPresetDirectory();
	std::string templatePath = system::join(presetDir, "template.vcvm");
	system::remove(templatePath);
}

void ModuleWidget::clearTemplateDialog() {
	std::string message = string::f(string::translate("ModuleWidget.clearTemplateDialog"), model->getFullName());
	if (!osdialog_message(OSDIALOG_INFO, OSDIALOG_OK_CANCEL, message.c_str()))
		return;
	clearTemplate();
}

void ModuleWidget::saveDialog() {
	std::string presetDir = model->getUserPresetDirectory();
	system::createDirectories(presetDir);

	// Delete directories if empty
	DEFER({
		try {
			system::remove(presetDir);
			system::remove(system::getDirectory(presetDir));
		}
		catch (Exception& e) {
			// Ignore exceptions if directory cannot be removed.
		}
	});

	osdialog_filters* filters = osdialog_filters_parse(PRESET_FILTERS);
	DEFER({osdialog_filters_free(filters);});

	char* pathC = osdialog_file(OSDIALOG_SAVE, presetDir.c_str(), "Untitled.vcvm", filters);
	if (!pathC) {
		// No path selected
		return;
	}
	DEFER({std::free(pathC);});

	std::string path = pathC;
	// Automatically append .vcvm extension
	if (system::getExtension(path) != ".vcvm")
		path += ".vcvm";

	save(path);
}

void ModuleWidget::disconnect() {
	for (PortWidget* pw : getPorts()) {
		getRack()->clearCablesOnPort(pw);
	}
}

void ModuleWidget::resetAction() {
	assert(module);

	// history::ModuleChange
	history::ModuleChange* h = new history::ModuleChange;
	h->name = string::translate("ModuleWidget.history.resetModule");
	h->moduleId = module->id;
	h->oldModuleJ = toJson();

	getEngine()->resetModule(module);

	h->newModuleJ = toJson();
	getHistory()->push(h);
}

void ModuleWidget::randomizeAction() {
	assert(module);

	// history::ModuleChange
	history::ModuleChange* h = new history::ModuleChange;
	h->name = string::translate("ModuleWidget.history.randomizeModule");
	h->moduleId = module->id;
	h->oldModuleJ = toJson();

	getEngine()->randomizeModule(module);

	h->newModuleJ = toJson();
	getHistory()->push(h);
}

void ModuleWidget::appendDisconnectActions(history::ComplexAction* complexAction) {
	for (PortWidget* pw : getPorts()) {
		for (CableWidget* cw : getRack()->getCompleteCablesOnPort(pw)) {
			// history::CableRemove
			history::CableRemove* h = new history::CableRemove;
			h->setCable(cw);
			complexAction->push(h);
			// Delete cable
			getRack()->removeCable(cw);
			delete cw;
		}
	};
}

void ModuleWidget::disconnectAction() {
	history::ComplexAction* complexAction = new history::ComplexAction;
	complexAction->name = string::translate("ModuleWidget.history.disconnectCables");
	appendDisconnectActions(complexAction);

	if (!complexAction->isEmpty())
		getHistory()->push(complexAction);
	else
		delete complexAction;
}

void ModuleWidget::cloneAction(bool cloneCables) {
	// history::ComplexAction
	history::ComplexAction* h = new history::ComplexAction;
	h->name = string::translate("ModuleWidget.history.duplicateModule");

	// Save patch store in this module so we can copy it below
	getEngine()->prepareSaveModule(module);

	// JSON serialization is the obvious way to do this
	json_t* moduleJ = toJson();
	DEFER({
		json_decref(moduleJ);
	});
	engine::Module::jsonStripIds(moduleJ);

	// Clone Module
	INFO("Creating module %s", model->getFullName().c_str());
	engine::Module* clonedModule = model->createModule();

	// Set ID here so we can copy module storage dir
	clonedModule->id = random::u64() % (1ull << 53);
	system::copy(module->getPatchStorageDirectory(), clonedModule->getPatchStorageDirectory());

	// This doesn't need a lock (via Engine::moduleFromJson()) because the Module is not added to the Engine yet.
	try {
		clonedModule->fromJson(moduleJ);
	}
	catch (Exception& e) {
		WARN("%s", e.what());
	}
	getEngine()->addModule(clonedModule);

	// Clone ModuleWidget
	INFO("Creating module widget %s", model->getFullName().c_str());
	ModuleWidget* clonedModuleWidget = model->createModuleWidget(clonedModule);
	getRack()->updateModuleOldPositions();
	getRack()->addModule(clonedModuleWidget);
	// Place module to the right of `this` module, by forcing it to 1 HP to the right.
	math::Vec clonedPos = getPos();
	clonedPos.setX(clonedPos.getX() + clonedModuleWidget->getWidth());
	if (settings::squeezeModules)
		getRack()->squeezeModulePos(clonedModuleWidget, clonedPos);
	else
		getRack()->setModulePosNearest(clonedModuleWidget, clonedPos);
	h->push(getRack()->getModuleDragAction());
	getRack()->updateExpanders();

	// history::ModuleAdd
	history::ModuleAdd* hma = new history::ModuleAdd;
	hma->setModule(clonedModuleWidget);
	h->push(hma);

	if (cloneCables) {
		// Clone cables attached to input/output ports
		for (PortWidget* pw : getPorts()) {
			for (CableWidget* cw : getRack()->getCompleteCablesOnPort(pw)) {
				// Skip input ports self-patched to this module's outputs, to avoid double-cloning them.
				if (pw->type == engine::Port::OUTPUT && cw->cable_->inputModule == module)
					continue;

				// Create cable attached to cloned ModuleWidget's input
				engine::Cable* clonedCable = new engine::Cable;
				clonedCable->inputModule = cw->cable_->inputModule;
				clonedCable->inputId = cw->cable_->inputId;
				clonedCable->outputModule = cw->cable_->outputModule;
				clonedCable->outputId = cw->cable_->outputId;

				if (pw->type == engine::Port::INPUT) {
					clonedCable->inputModule = clonedModule;
					// If cable is self-patched, attach to cloned module instead
					if (cw->cable_->outputModule == module)
						clonedCable->outputModule = clonedModule;
				}
				else {
					clonedCable->outputModule = clonedModule;
				}

				getEngine()->addCable(clonedCable);

				app::CableWidget* clonedCw = new app::CableWidget;
				clonedCw->setCable(clonedCable);
				clonedCw->setColor(cw->getColor());
				getRack()->addCable(clonedCw);

				// history::CableAdd
				history::CableAdd* hca = new history::CableAdd;
				hca->setCable(clonedCw);
				h->push(hca);
			}
		}
	}

	getHistory()->push(h);
}

void ModuleWidget::bypassAction(bool bypassed) {
	assert(module);

	// history::ModuleBypass
	history::ModuleBypass* h = new history::ModuleBypass;
	h->moduleId = module->id;
	h->bypassed = bypassed;
	if (!bypassed)
		h->name = string::translate("ModuleWidget.history.unbypassModule");
	getHistory()->push(h);

	getEngine()->bypassModule(module, bypassed);
}

void ModuleWidget::removeAction() {
	history::ComplexAction* h = new history::ComplexAction;
	h->name = string::translate("ModuleWidget.history.deleteModule");

	// Disconnect cables
	appendDisconnectActions(h);

	// Unset module position from rack.
	getRack()->updateModuleOldPositions();
	if (settings::squeezeModules)
		getRack()->unsqueezeModulePos(this);
	h->push(getRack()->getModuleDragAction());

	// history::ModuleRemove
	history::ModuleRemove* moduleRemove = new history::ModuleRemove;
	moduleRemove->setModule(this);
	h->push(moduleRemove);

	getHistory()->push(h);

	// This removes the module and transfers ownership to caller
	getRack()->removeModule(this);
	delete this;

	getRack()->updateExpanders();
}


// Create ModulePresetPathItems for each patch in a directory.
static void appendPresetItems(ui::Menu* menu, WeakPtr<ModuleWidget> moduleWidget, std::string presetDir) {
	bool hasPresets = false;
	if (system::isDirectory(presetDir)) {
		// Note: This is not cached, so opening this menu each time might have a bit of latency.
		std::vector<std::string> entries = system::getEntries(presetDir);
		std::sort(entries.begin(), entries.end());
		for (std::string path : entries) {
			std::string name = system::getStem(path);
			// Remove "1_", "42_", "001_", etc at the beginning of preset filenames
			std::regex r("^\\d+_");
			name = std::regex_replace(name, r, "");

			if (system::isDirectory(path)) {
				hasPresets = true;

				menu->addChild(createSubmenuItem(name, "", [=](ui::Menu* menu) {
					if (!moduleWidget)
						return;
					appendPresetItems(menu, moduleWidget, path);
				}));
			}
			else if (system::getExtension(path) == ".vcvm" && name != "template") {
				hasPresets = true;

				menu->addChild(createMenuItem(name, "", [=]() {
					if (!moduleWidget)
						return;
					try {
						moduleWidget->loadAction(path);
					}
					catch (Exception& e) {
						osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, e.what());
					}
				}));
			}
		}
	}
	if (!hasPresets) {
		menu->addChild(createMenuLabel(string::translate("ModuleWidget.nonePresets")));
	}
};

void ModuleWidget::createContextMenu() {
    ui::Menu* menu = createMenu();
    assert(model);

    WeakPtr<ModuleWidget> weakThis = this;

    // Brand and module name
    menu->addChild(createMenuLabel(model->name));
    menu->addChild(createMenuLabel(model->plugin->brand));

    // Info
    menu->addChild(createSubmenuItem(
        string::translate("ModuleWidget.info"), "", [=](ui::Menu* menu) {
            model->appendContextMenu(menu);

            if (!weakThis) return;
            menu->addChild(new ui::MenuSeparator);
            menu->addChild(
                createMenuLabel(string::translate("ModuleWidget.moduleId")));
            menu->addChild(createMenuLabel(
                string::f("%lld", (long long)weakThis->module->getId())));
        }));

    // Preset
    menu->addChild(createSubmenuItem(
        string::translate("ModuleWidget.preset"), "", [=](ui::Menu* menu) {
            menu->addChild(createMenuItem(
                string::translate("ModuleWidget.copy"),
                widget::getKeyCommandName(GLFW_KEY_C, RACK_MOD_CTRL), [=]() {
                    if (!weakThis) return;
                    weakThis->copyClipboard();
                }));

            menu->addChild(createMenuItem(
                string::translate("ModuleWidget.paste"),
                widget::getKeyCommandName(GLFW_KEY_V, RACK_MOD_CTRL), [=]() {
                    if (!weakThis) return;
                    weakThis->pasteClipboardAction();
                }));

            menu->addChild(createMenuItem(
                string::translate("ModuleWidget.load"), "", [=]() {
                    if (!weakThis) return;
                    weakThis->loadDialog();
                }));

            menu->addChild(createMenuItem(
                string::translate("ModuleWidget.saveAs"), "", [=]() {
                    if (!weakThis) return;
                    weakThis->saveDialog();
                }));

            menu->addChild(createMenuItem(
                string::translate("ModuleWidget.saveTemplate"), "", [=]() {
                    if (!weakThis) return;
                    weakThis->saveTemplateDialog();
                }));

            menu->addChild(createMenuItem(
                string::translate("ModuleWidget.clearTemplate"), "",
                [=]() {
                    if (!weakThis) return;
                    weakThis->clearTemplateDialog();
                },
                !weakThis->hasTemplate()));

            // Scan `<user dir>/presets/<plugin slug>/<module slug>` for
            // presets.
            menu->addChild(new ui::MenuSeparator);
            menu->addChild(
                createMenuLabel(string::translate("ModuleWidget.userPresets")));
            appendPresetItems(menu, weakThis,
                              weakThis->model->getUserPresetDirectory());

            // Scan `<plugin dir>/presets/<module slug>` for presets.
            menu->addChild(new ui::MenuSeparator);
            menu->addChild(createMenuLabel(
                string::translate("ModuleWidget.factoryPresets")));
            appendPresetItems(menu, weakThis,
                              weakThis->model->getFactoryPresetDirectory());
        }));

    // Initialize
    menu->addChild(createMenuItem(
        string::translate("ModuleWidget.initialize"),
        widget::getKeyCommandName(GLFW_KEY_I, RACK_MOD_CTRL), [=]() {
            if (!weakThis) return;
            weakThis->resetAction();
        }));

    // Randomize
    menu->addChild(createMenuItem(
        string::translate("ModuleWidget.randomize"),
        widget::getKeyCommandName(GLFW_KEY_R, RACK_MOD_CTRL), [=]() {
            if (!weakThis) return;
            weakThis->randomizeAction();
        }));

    // Disconnect cables
    menu->addChild(createMenuItem(
        string::translate("ModuleWidget.disconnectCables"),
        widget::getKeyCommandName(GLFW_KEY_U, RACK_MOD_CTRL), [=]() {
            if (!weakThis) return;
            weakThis->disconnectAction();
        }));

    // Bypass
    std::string bypassText =
        widget::getKeyCommandName(GLFW_KEY_E, RACK_MOD_CTRL);
    bool bypassed = module && module->isBypassed();
    if (bypassed) bypassText += " " CHECKMARK_STRING;
    menu->addChild(createMenuItem(string::translate("ModuleWidget.bypass"),
                                  bypassText, [=]() {
                                      if (!weakThis) return;
                                      weakThis->bypassAction(!bypassed);
                                  }));

    // Duplicate
    menu->addChild(createMenuItem(
        string::translate("ModuleWidget.duplicate"),
        widget::getKeyCommandName(GLFW_KEY_D, RACK_MOD_CTRL), [=]() {
            if (!weakThis) return;
            weakThis->cloneAction(false);
        }));

    // Duplicate with cables
    menu->addChild(createMenuItem(
        "└ " + string::translate("ModuleWidget.duplicateWithCables"),
        widget::getKeyCommandName(GLFW_KEY_D, RACK_MOD_CTRL | GLFW_MOD_SHIFT),
        [=]() {
            if (!weakThis) return;
            weakThis->cloneAction(true);
        }));

    // Delete
    menu->addChild(createMenuItem(
        string::translate("ModuleWidget.delete"),
        ui::Liminal::hasKeyboard() ? 
        widget::getKeyCommandName(GLFW_KEY_BACKSPACE, 0) + "/" +
            widget::getKeyCommandName(GLFW_KEY_DELETE, 0) : "",
        [=]() {
            if (!weakThis) return;
            weakThis->removeAction();
        },
        false, true));

    // Zoom to fit
    menu->addChild(createMenuItem(
        string::translate("ModuleWidget.zoomFit"),
        widget::getKeyCommandName(GLFW_KEY_F4, RACK_MOD_CTRL), [=]() {
            if (!weakThis) return;
            getScene()->getRackScroll()->zoomToBound(weakThis->getBox());
        }));

    appendContextMenu(menu);
}

math::Vec ModuleWidget::getGridPosition() {
	return ((getPos() - RACK_OFFSET) / RACK_GRID_SIZE).round();
}

void ModuleWidget::setGridPosition(math::Vec pos) {
	setPos(pos * RACK_GRID_SIZE + RACK_OFFSET);
}

math::Vec ModuleWidget::getGridSize() {
	return (getSize() / RACK_GRID_SIZE).round();
}

math::Rect ModuleWidget::getGridBox() {
	return math::Rect(getGridPosition(), getGridSize());
}

math::Vec& ModuleWidget::dragOffset() {
	return internal_->dragOffset;
}

bool& ModuleWidget::dragEnabled() {
	return internal_->dragEnabled;
}

engine::Module* ModuleWidget::releaseModule() {
	engine::Module* module = this->module;
	this->module = NULL;
	return module;
}


} // namespace app
} // namespace rack