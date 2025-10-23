#include <app/PortWidget.hpp>
#include <app/Scene.hpp>
#include <app/CableColorMatcher.hpp>
#include <ui/MenuItem.hpp>
#include <ui/MenuSeparator.hpp>
#include <window/Window.hpp>
#include <context.hpp>
#include <history.hpp>
#include <engine/Engine.hpp>
#include <settings.hpp>
#include <helpers.hpp>


namespace rack {
namespace app {


struct PortWidget::Internal {
	ui::Tooltip* tooltip = NULL;
	/** For overriding onDragStart behavior by menu items. */
	std::vector<CableWidget*> overrideCws;
	CableWidget* overrideCloneCw = NULL;
	bool overrideCreateCable = false;
	/** When dragging port, this is the grabbed end type of the cable. */
	engine::Port::Type draggedType = engine::Port::INPUT;
	/** Created when dragging starts, deleted when it ends. */
	history::ComplexAction* history = NULL;
};


struct PortTooltip : ui::Tooltip {
	PortWidget* portWidget;

	void step() override {
		if (portWidget->module) {
			engine::Port* port = portWidget->getPort();
			engine::PortInfo* portInfo = portWidget->getPortInfo();
			// Label
			text = portInfo->getFullName();
			// Description
			std::string description = portInfo->getDescription();
			if (description != "") {
				text += "\n";
				text += description;
			}
			// Voltage, number of channels
			int channels = port->getChannels();
			for (int i = 0; i < channels; i++) {
				float v = port->getVoltage(i);
				// Add newline or comma
				text += "\n";
				if (channels > 1)
					text += string::f("%d: ", i + 1);
				text += string::f("% .3fV", math::normalizeZero(v));
			}
			// From/To
			std::vector<CableWidget*> cables = getRack()->getCompleteCablesOnPort(portWidget);
			for (auto it = cables.rbegin(); it != cables.rend(); it++) {
				CableWidget* cable = *it;
				PortWidget* otherPw = (portWidget->type == engine::Port::INPUT) ? cable->outputPort : cable->inputPort;
				if (!otherPw)
					continue;
				text += "\n";
				if (portWidget->type == engine::Port::INPUT)
					text += string::translate("PortWidget.from");
				else
					text += string::translate("PortWidget.to");
				text += otherPw->module->model->getFullName();
				text += ": ";
				text += otherPw->getPortInfo()->getName();
				text += " ";
				text += (otherPw->type == engine::Port::INPUT) ? string::translate("PortWidget.input") : string::translate("PortWidget.output");
			}
		}

        // Use Tooltip step() to size and position the tooltip
		Tooltip::step();

		// Fit inside parent (copied from Tooltip.cpp)
		assert(getParent());
		setBox(getBox().nudge(getParent()->getBox().zeroPos()));
	}
};


struct PortCloneCableItem : ui::MenuItem {
	PortWidget* pw;
	CableWidget* cw;

	void onButton(const ButtonEvent& e) override {
		OpaqueWidget::onButton(e);
		if (isDisabled())
			return;
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			// Set PortWidget::onDragStart overrides
			pw->internal_->overrideCloneCw = cw;

			// Pretend the PortWidget was clicked
			e.consume(pw);
			// Deletes `this`
			doAction();
		}
	}
};


struct CableColorItem : ui::ColorDotMenuItem {
	CableWidget* cw;

	void onAction(const ActionEvent& e) override {
		// history::CableColorChange
		history::CableColorChange* h = new history::CableColorChange;
		h->setCable(cw);
		h->newColor = color;
		h->oldColor = cw->color;
		getHistory()->push(h);

		cw->color = color;
	}
};


struct PortCableItem : ui::ColorDotMenuItem {
	PortWidget* pw;
	CableWidget* cw;

	void onButton(const ButtonEvent& e) override {
		OpaqueWidget::onButton(e);
		if (isDisabled())
			return;
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			// Set PortWidget::onDragStart overrides
			pw->internal_->overrideCws.push_back(cw);

			// Pretend the PortWidget was clicked
			e.consume(pw);
			// Deletes `this`
			doAction();
		}
	}

	ui::Menu* createChildMenu() override {
		ui::Menu* menu = new ui::Menu;

		// menu->addChild(createMenuLabel(string::f(string::translate("PortWidget.cableId"), cw->cable->id)));

		for (NVGcolor color : settings::cableColors) {
			// Include extra leading spaces for the color circle
			CableColorItem* item = createMenuItem<CableColorItem>(string::translate("PortWidget.setColor"));
			item->setDisabled(color::isEqual(color, cw->color));
			item->cw = cw;
			item->color = color;
			menu->addChild(item);
		}

		return menu;
	}
};


struct PortAllCablesItem : ui::MenuItem {
	PortWidget* pw;
	std::vector<CableWidget*> cws;

	void onButton(const ButtonEvent& e) override {
		OpaqueWidget::onButton(e);
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			// Set PortWidget::onDragStart overrides
			pw->internal_->overrideCws = cws;

			// Pretend the PortWidget was clicked
			e.consume(pw);
			// Deletes `this`
			doAction();
		}
	}
};


struct PortCreateCableItem : ui::MenuItem {
	PortWidget* pw;

	void onButton(const ButtonEvent& e) override {
		OpaqueWidget::onButton(e);
		if (isDisabled())
			return;
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			// Set PortWidget::onDragStart overrides
			pw->internal_->overrideCreateCable = true;

			// Pretend the PortWidget was clicked
			e.consume(pw);
			// Deletes `this`
			doAction();
		}
	}
};


struct PortCreateCableColorItem : ui::ColorDotMenuItem {
	PortWidget* pw;
	size_t colorId;

	void onButton(const ButtonEvent& e) override {
		OpaqueWidget::onButton(e);
		if (isDisabled())
			return;
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			getRack()->setNextCableColorId(colorId);
			// Set PortWidget::onDragStart overrides
			pw->internal_->overrideCreateCable = true;

			// Pretend the PortWidget was clicked
			e.consume(pw);
			// Deletes `this`
			doAction();
		}
	}
};


PortWidget::PortWidget() {
	internal_ = new Internal;
}


PortWidget::~PortWidget() {
	// The port shouldn't have any cables when destroyed, but just to make sure.
	if (module)
		getRack()->clearCablesOnPort(this);
	// HACK: In case onDragDrop() is called but not onLeave() afterwards...
	destroyTooltip();
	delete internal_;
}


engine::Port* PortWidget::getPort() {
	if (!module)
		return NULL;
	if (type == engine::Port::INPUT)
		return &module->inputs[portId];
	else
		return &module->outputs[portId];
}


engine::PortInfo* PortWidget::getPortInfo() {
	if (!module)
		return NULL;
	if (type == engine::Port::INPUT)
		return module->inputInfos[portId];
	else
		return module->outputInfos[portId];
}


void PortWidget::createTooltip() {
	// If tooltips disabled, do not create tooltip
	if (!settings::tooltips)
		return;

	// If tooltip already exists, do not create another one
	if (internal_->tooltip)
		return;

	if (!module)
		return;

	// Create tooltip
	PortTooltip* tooltip = new PortTooltip;
	tooltip->portWidget = this;
	getScene()->addChild(tooltip);
	internal_->tooltip = tooltip;
}


void PortWidget::destroyTooltip() {
	// If no tooltip don't need to destroy it
	if (!internal_->tooltip)
		return;

	// Actually destroy the tooltip
	getScene()->removeChild(internal_->tooltip);
	delete internal_->tooltip;
	internal_->tooltip = NULL;
}


void PortWidget::createContextMenu() {
	ui::Menu* menu = createMenu();
	WeakPtr<PortWidget> weakThis = this;

	engine::PortInfo* portInfo = getPortInfo();
	assert(portInfo);
	menu->addChild(createMenuLabel(portInfo->getFullName()));

	std::vector<CableWidget*> cws = getRack()->getCompleteCablesOnPort(this);
	CableWidget* topCw = cws.empty() ? NULL : cws.back();

	menu->addChild(createMenuItem(string::translate("PortWidget.deleteTopCable"), widget::getKeyCommandName(0, RACK_MOD_SHIFT) + string::translate("key.click"),
		[=]() {
			if (!weakThis)
				return;
			weakThis->deleteTopCableAction();
		},
		!topCw
	));

	{
		PortCloneCableItem* item = createMenuItem<PortCloneCableItem>(string::translate("PortWidget.cloneTopCable"), widget::getKeyCommandName(0, RACK_MOD_CTRL | RACK_MOD_SHIFT) + string::translate("key.drag"));
		item->setDisabled(!topCw);
		item->pw = this;
		item->cw = topCw;
		menu->addChild(item);
	}

	{
		PortCreateCableItem* item = createMenuItem<PortCreateCableItem>(string::translate("PortWidget.createCableTop"), widget::getKeyCommandName(0, RACK_MOD_CTRL) + string::translate("key.drag"));
		item->pw = this;
		menu->addChild(item);
	}

	menu->addChild(new ui::MenuSeparator);

	// New cable items
	for (size_t colorId = 0; colorId < settings::cableColors.size(); colorId++) {
		NVGcolor color = settings::cableColors[colorId];
		std::string label = get(settings::cableLabels, colorId);
		if (label == "")
			label = string::f("#%lld", (long long) (colorId + 1));
		PortCreateCableColorItem* item = createMenuItem<PortCreateCableColorItem>(string::translate("PortWidget.createCable") + label);
		item->pw = this;
		item->color = color;
		item->colorId = colorId;
		menu->addChild(item);
	}

	if (!cws.empty()) {
		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuLabel(string::translate("key.clickDrag") + string::translate("PortWidget.grabCable")));

		// Cable items
		for (auto it = cws.rbegin(); it != cws.rend(); it++) {
			CableWidget* cw = *it;
			PortWidget* pw = (type == engine::Port::INPUT) ? cw->outputPort : cw->inputPort;
			engine::PortInfo* portInfo = pw->getPortInfo();

			PortCableItem* item = createMenuItem<PortCableItem>(portInfo->module->model->name + ": " + portInfo->getName(), RIGHT_ARROW);
			item->color = cw->color;
			item->pw = this;
			item->cw = cw;
			menu->addChild(item);
		}

		if (cws.size() > 1) {
			PortAllCablesItem* item = createMenuItem<PortAllCablesItem>(string::translate("PortWidget.allCables"));
			item->pw = this;
			item->cws = cws;
			menu->addChild(item);
		}
	}

	appendContextMenu(menu);
}


void PortWidget::deleteTopCableAction() {
	CableWidget* cw = getRack()->getTopCable(this);
	if (!cw)
		return;

	// history::CableRemove
	history::CableRemove* h = new history::CableRemove;
	h->setCable(cw);
	getHistory()->push(h);

	getRack()->removeCable(cw);
	delete cw;
}


void PortWidget::step() {
	Widget::step();
}

void PortWidget::draw(const DrawArgs& args) {
    // Check if left-dragging a PortWidget
    PortWidget* draggedPw = dynamic_cast<PortWidget*>(getEvent()->getDraggedWidget());
    if (draggedPw && getEvent()->dragButton == GLFW_MOUSE_BUTTON_LEFT) {
        // Dragging a cable, which means should emphasize ports that can be connected to
        // and deemphasize ports that cannot be connected to. Use nvtTint to change the
        // colors and alpha used to draw the ports.
        if (draggedPw->internal_->draggedType != type) {
            // Cannot make a connection to the port so deemphasize it. This is accomplished
            // by reducing alpha to 0.4, which basically makes the port fade out
            nvgTint(args.vg, nvgRGBAf(1.0, 1.0, 1.0, 0.5));
        } else {
            // Can make a connection to the port so emphasize it
            if (draggedPw->internal_->draggedType == engine::Port::OUTPUT) {
                // It is an output port so emphasize it with a gold color.
				// Note: if instead want to emphasize red then should use
                // nvgTint(args.vg, nvgRGBAf(1.0, 0.7, 0.7, 1.0));
                nvgTint(args.vg, nvgRGBAf(1.1, 1.2, 0.5, 1.0));
            } else {
                // Emphasize port as an INPUT port by keeping
                // green color high but reducing red and blue. Keeping
                // alpha at 1.0. Result is sthat the port will still be
                // pretty bright but will also be noticably green.
                nvgTint(args.vg, nvgRGBAf(0.7, 1.0, 0.7, 1.0));
            }
        }
    }

	// Draw port with appropriate tinting
    Widget::draw(args);
}

void PortWidget::onButton(const ButtonEvent& e) {
	OpaqueWidget::onButton(e);

	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
		createContextMenu();
		e.consume(this);
		return;
	}

	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
		deleteTopCableAction();
		// Consume null so onDragStart isn't triggered
		e.consume(NULL);
		return;
	}
}


void PortWidget::onEnter(const EnterEvent& e) {
	createTooltip();
}


void PortWidget::onLeave(const LeaveEvent& e) {
	destroyTooltip();
}


void PortWidget::onDragStart(const DragStartEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	DEFER({
		// Reset overrides
		internal_->overrideCws.clear();
		internal_->overrideCloneCw = NULL;
		internal_->overrideCreateCable = false;
	});

	// Create ComplexAction
	if (internal_->history) {
		delete internal_->history;
		internal_->history = NULL;
	}
	internal_->history = new history::ComplexAction;
	internal_->history->name = string::translate("PortWidget.history.moveCable");

	std::vector<CableWidget*> cws;
	int mods = getWindow()->getMods();
	if (internal_->overrideCreateCable || (mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
		// Create cable with Ctrl+drag or PortCreateCableItem
		// Keep cable NULL. Will be created below
	}
	else if (internal_->overrideCloneCw || (mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
		// Clone top cable with Ctrl+shift+drag or PortCloneCableItem
		CableWidget* cloneCw = internal_->overrideCloneCw;
		if (!cloneCw)
			cloneCw = getRack()->getTopCable(this);

		if (cloneCw) {
			CableWidget* cw = new CableWidget;
			cw->color = cloneCw->color;
			if (type == engine::Port::OUTPUT)
				cw->inputPort = cloneCw->inputPort;
			else
				cw->outputPort = cloneCw->outputPort;
			internal_->draggedType = type;
			getRack()->addCable(cw);
			cws.push_back(cw);
		}
	}
	else {
		// Grab cable on top of stack
		cws = internal_->overrideCws;
		if (cws.empty()) {
			CableWidget* cw = getRack()->getTopCable(this);
			if (cw)
				cws.push_back(cw);
		}

		for (CableWidget* cw : cws) {
			// history::CableRemove
			history::CableRemove* h = new history::CableRemove;
			h->setCable(cw);
			internal_->history->push(h);

			// Reuse existing cable
			cw->getPort(type) = NULL;
			cw->updateCable();
			internal_->draggedType = type;

			// Move grabbed plug to top of stack
			PlugWidget* plug = cw->getPlug(type);
			assert(plug);
			getRack()->getPlugContainer()->removeChild(plug);
			getRack()->getPlugContainer()->addChild(plug);
		}
	}

	// If not using existing cable, create new cable
	if (cws.empty()) {
		CableWidget* cw = new CableWidget;

		// Set color of the cable
        cw->color = CableColorMatcher::getCableColor(getPortInfo());

        // Set port
		cw->getPort(type) = this;
		internal_->draggedType = (type == engine::Port::INPUT) ? engine::Port::OUTPUT : engine::Port::INPUT;
		getRack()->addCable(cw);
	}
}


void PortWidget::onDragEnd(const DragEndEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	// Remove all incomplete cables
	for (CableWidget* cw : getRack()->getIncompleteCables()) {
		getRack()->removeCable(cw);
		delete cw;
	}

	// Push history
	if (!internal_->history) {
		// This shouldn't happen since it's created in onDragStart()
	}
	else if (internal_->history->isEmpty()) {
		// No history actions, don't push anything
		delete internal_->history;
	}
	else if (internal_->history->actions.size() == 1) {
		// Push single history action
		getHistory()->push(internal_->history->actions[0]);
		internal_->history->actions.clear();
		delete internal_->history;
	}
	else {
		// Push ComplexAction
		getHistory()->push(internal_->history);
	}
	internal_->history = NULL;
}


void PortWidget::onDragDrop(const DragDropEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	PortWidget* pwOrigin = dynamic_cast<PortWidget*>(e.origin);
	if (!pwOrigin)
		return;

	// HACK: Only delete tooltip if we're not (normal) dragging it.
	if (pwOrigin == this) {
		createTooltip();
	}

	for (CableWidget* cw : getRack()->getIncompleteCables()) {
		// These should already be NULL because onDragLeave() is called immediately before onDragDrop().
		cw->hoveredOutputPort = NULL;
		cw->hoveredInputPort = NULL;
		if (type == engine::Port::OUTPUT) {
			// Check that similar cable doesn't exist
			if (cw->inputPort && !getRack()->getCable(this, cw->inputPort)) {
				// Connecting to output port so remember this port
				cw->outputPort = this;
			}
			else {
				// Cable already exists so skip this one
				continue;
			}
		}
		else {
			if (cw->outputPort && !getRack()->getCable(cw->outputPort, this)) {
				// Connecting to input port so remember this port
				cw->inputPort = this;
			}
			else {
				// Cable already exists so skip this one
				continue;
			}
		}

		// Input and output ports are set, so finish updating the cable
		cw->updateCable();

		// This should always be true since the ComplexAction is created in onDragStart()
		history::ComplexAction* history = pwOrigin->internal_->history;
		if (history) {
			// Reject history if plugging into same port
			auto& actions = history->actions;
			auto it = std::find_if(actions.begin(), actions.end(), [&](history::Action* h) {
				history::CableAdd* hca = dynamic_cast<history::CableAdd*>(h);
				if (!hca)
					return false;
				return hca->isCable(cw);
			});

			if (it != actions.end()) {
				actions.erase(it);
			}
			else {
				// Push CableAdd action
				history::CableAdd* h = new history::CableAdd;
				h->setCable(cw);
				history->push(h);
			}
		}
	}
}


void PortWidget::onDragEnter(const DragEnterEvent& e) {
	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	// Check if dragging from another port, which implies that a cable is being dragged
	PortWidget* pwOrigin = dynamic_cast<PortWidget*>(e.origin);
	if (!pwOrigin)
		return;

	createTooltip();

	// Make all incomplete cables hover this port
	for (CableWidget* cw : getRack()->getIncompleteCables()) {
		if (type == engine::Port::OUTPUT) {
			// Check that similar cable doesn't exist
			if (cw->inputPort && !getRack()->getCable(this, cw->inputPort)) {
				cw->hoveredOutputPort = this;
			}
		}
		else {
			if (cw->outputPort && !getRack()->getCable(cw->outputPort, this)) {
				cw->hoveredInputPort = this;
			}
		}
	}
}


void PortWidget::onDragLeave(const DragLeaveEvent& e) {
	destroyTooltip();

	if (e.button != GLFW_MOUSE_BUTTON_LEFT)
		return;

	PortWidget* pwOrigin = dynamic_cast<PortWidget*>(e.origin);
	if (!pwOrigin)
		return;

	for (CableWidget* cw : getRack()->getIncompleteCables()) {
		cw->getHoveredPort(type) = NULL;
	}
}


} // namespace app
} // namespace rack
