#include <app/ParamWidget.hpp>
#include <ui/MenuOverlay.hpp>
#include <ui/MenuSeparator.hpp>
#include <ui/TextField.hpp>
#include <app/Knob.hpp>
#include <app/Scene.hpp>
#include <context.hpp>
#include <engine/Engine.hpp>
#include <engine/ParamQuantity.hpp>
#include <settings.hpp>
#include <history.hpp>
#include <helpers.hpp>


namespace rack {
namespace app {


struct ParamField : ui::TextField {
	ParamWidget* paramWidget;

	void step() override {
		// Keep selected
		getEvent()->setSelectedWidget(this);
		TextField::step();
	}

	void setParamWidget(ParamWidget* paramWidget) {
		this->paramWidget = paramWidget;
		engine::ParamQuantity* pq = paramWidget->getParamQuantity();
		if (pq)
			setText(pq->getDisplayValueString());
		selectAll();
	}

	void onSelectKey(const SelectKeyEvent& e) override {
		if (e.action == GLFW_PRESS && (e.isKeyCommand(GLFW_KEY_ENTER) || e.isKeyCommand(GLFW_KEY_KP_ENTER))) {
			engine::ParamQuantity* pq = paramWidget->getParamQuantity();
			assert(pq);
			float oldValue = pq->getValue();
			if (pq)
				pq->setDisplayValueString(getText());
			float newValue = pq->getValue();

			if (oldValue != newValue) {
				// Push ParamChange history action
				history::ParamChange* h = new history::ParamChange;
				h->moduleId = paramWidget->module->id;
				h->paramId = paramWidget->paramId;
				h->oldValue = oldValue;
				h->newValue = newValue;
				getHistory()->push(h);
			}

			ui::MenuOverlay* overlay = getAncestorOfType<ui::MenuOverlay>();
			overlay->requestDelete();
			e.consume(this);
		}

		if (!e.getTarget())
			TextField::onSelectKey(e);
	}
};


class ParamValueItem : public ui::MenuItem {
public:
 ParamValueItem(const std::string& text = "", const std::string& name = "",
                ParamWidget* paramWidgetPtr = nullptr)
     : ui::MenuItem(text, name), paramWidget(paramWidgetPtr) {}

 void setParamWidget(ParamWidget* paramWidgetPtr) {
     paramWidget = paramWidgetPtr;
 }

    // Could be used by module code so need to leave it as public
    float value;

private:
    ParamWidget* paramWidget;

    void onAction(const ActionEvent& e) override {
        if (!paramWidget)
            return;
        engine::ParamQuantity* pq = paramWidget->getParamQuantity();
        if (pq) {
            float oldValue = pq->getValue();
            pq->setValue(value);
            float newValue = pq->getValue();

            if (oldValue != newValue) {
                // Push ParamChange history action
                history::ParamChange* h = new history::ParamChange;
                h->name = string::translate("ParamWidget.history.setParam");
                h->moduleId = paramWidget->module->id;
                h->paramId = paramWidget->paramId;
                h->oldValue = oldValue;
                h->newValue = newValue;
                getHistory()->push(h);
            }
        }
    }
};

class ParamTooltip : public ui::Tooltip {
   public:
    ParamTooltip(ParamWidget& paramWidgetRef) : paramWidget(paramWidgetRef) {}

   private:
    void step() override {
        engine::ParamQuantity* pq = paramWidget.getParamQuantity();
        if (pq) {
            // Quantity string
            text = pq->getString();
            // Description
            std::string description = pq->getDescription();
            if (description != "") {
                text += "\n";
                text += description;
            }
        }

        // Use Tooltip step() to size and position the tooltip
        Tooltip::step();

        // Fit inside parent (copied from Tooltip.cpp).
        // Seems to not actually do anything though.
        assert(getParent());
        setBox(getBox().nudge(getParent()->getBox().zeroPos()));
    }

   private:
    ParamWidget& paramWidget;
};

class ParamLabel : public ui::MenuLabel {
   public:
    ParamLabel(ParamWidget& paramWidgetRef) : paramWidget(paramWidgetRef) {}

   private:
    ParamWidget& paramWidget;

    void step() override {
        engine::ParamQuantity* pq = paramWidget.getParamQuantity();
        setText(pq->getString());
        MenuLabel::step();
    }
};

engine::ParamQuantity* ParamWidget::getParamQuantity() {
	if (!module)
		return NULL;
	return module->paramQuantities[paramId];
}


struct ParamWidget::Internal {
	ui::Tooltip* tooltip = NULL;
	/** For triggering the Change event. `*/
	float lastValue = NAN;
};


ParamWidget::ParamWidget() {
	internal_ = new Internal;
}


ParamWidget::~ParamWidget() {
	delete internal_;
}


void ParamWidget::createTooltip() {
	if (!settings::tooltips)
		return;
	if (internal_->tooltip)
		return;
	if (!module)
		return;
	ParamTooltip* tooltip = new ParamTooltip(*this);
	getScene()->addChild(tooltip);
	internal_->tooltip = tooltip;
}


void ParamWidget::destroyTooltip() {
	if (!internal_->tooltip)
		return;
	getScene()->removeChild(internal_->tooltip);
	delete internal_->tooltip;
	internal_->tooltip = NULL;
}

void ParamWidget::step() {
	engine::ParamQuantity* pq = getParamQuantity();
	if (pq) {
		float value = pq->getValue();
		// Dispatch change event when the ParamQuantity value changes
		if (value != internal_->lastValue) {
			ChangeEvent eChange;
			onChange(eChange);
			internal_->lastValue = value;
		}
	}

	Widget::step();
}


void ParamWidget::draw(const DrawArgs& args) {
    // Call parent draw to actually draw the param widget
	Widget::draw(args);

	// Param map indicator
	engine::ParamHandle* paramHandle = module ? getEngine()->getParamHandle(module->id, paramId) : NULL;
	if (paramHandle) {
		NVGcolor color = paramHandle->color;
		nvgBeginPath(args.vg);
		const float radius = 6;
		// nvgCircle(args.vg, box.size.x / 2, box.size.y / 2, radius);
		nvgRect(args.vg, getWidth() - radius, getHeight() - radius, radius, radius);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, color::mult(color, 0.5));
		nvgStrokeWidth(args.vg, 1.0);
		nvgStroke(args.vg);
	}

    // Wanted to have Knob::draw() override this draw() function to add 3D effects,
    // but since 3rd-party plugins are compiled against the legacy Rack SDK, they
    // will not call an overriden function. So we create a new function drawOverride()
    // that does the work, and call that from here.
    // If this object is derived from Knob, call Knob::drawOverride().
    if (Knob* derived_obj = dynamic_cast<Knob*>(this))
        derived_obj->drawOverride(args);
}


void ParamWidget::onButton(const ButtonEvent& e) {
	OpaqueWidget::onButton(e);

	// If left click remember that this param was the one clicked on
	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
		if (module) {
			getRack()->setTouchedParam(this);
		}
		e.consume(this);
	}

	// If right click open context menu
	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && (e.mods & RACK_MOD_MASK) == 0) {
		destroyTooltip();
		createContextMenu();
		e.consume(this);
	}
}


void ParamWidget::onDoubleClick(const DoubleClickEvent& e) {
	resetAction();
}


void ParamWidget::onEnter(const EnterEvent& e) {
	createTooltip();
}


void ParamWidget::onLeave(const LeaveEvent& e) {
	destroyTooltip();
}


void ParamWidget::createContextMenu() {
	ui::Menu* menu = createMenu();

	engine::ParamQuantity* pq = getParamQuantity();
	engine::SwitchQuantity* switchQuantity = dynamic_cast<engine::SwitchQuantity*>(pq);

	ParamLabel* paramLabel = new ParamLabel(*this);
	menu->addChild(paramLabel);

	if (switchQuantity) {
		float minValue = pq->getMinValue();
		int index = (int) std::floor(pq->getValue() - minValue);
		int numStates = switchQuantity->labels.size();
		for (int i = 0; i < numStates; i++) {
			std::string label = switchQuantity->labels[i];
			ParamValueItem* paramValueItem = createMenuItem<ParamValueItem>(label, CHECKMARK(i == index));
            paramValueItem->setParamWidget(this);
			paramValueItem->value = minValue + i;
			menu->addChild(paramValueItem);
		}
		if (numStates > 0) {
			menu->addChild(new ui::MenuSeparator);
		}
	}
	else {
		ParamField* paramField = new ParamField;
		paramField->setWidth(100);
		paramField->setParamWidget(this);
		menu->addChild(paramField);
	}

	// Initialize
	if (pq && pq->resetEnabled && pq->isBounded()) {
		menu->addChild(createMenuItem(string::translate("ParamWidget.initialize"), switchQuantity ? "" : string::translate("key.doubleClick"), [=]() {
			this->resetAction();
		}));
	}

	// Fine
	if (!switchQuantity) {
		menu->addChild(createMenuItem(string::translate("ParamWidget.fine"), widget::getKeyCommandName(0, RACK_MOD_CTRL) + string::translate("key.drag"), NULL, true));
	}

	// Unmap
	engine::ParamHandle* paramHandle = module ? getEngine()->getParamHandle(module->id, paramId) : NULL;
	if (paramHandle) {
		menu->addChild(createMenuItem(string::translate("ParamWidget.unmap"), paramHandle->text, [=]() {
			getEngine()->updateParamHandle(paramHandle, -1, 0);
		}));
	}

	appendContextMenu(menu);
}


void ParamWidget::resetAction() {
	engine::ParamQuantity* pq = getParamQuantity();
	if (pq && pq->resetEnabled && pq->isBounded()) {
		float oldValue = pq->getValue();
		pq->reset();
		float newValue = pq->getValue();

		if (oldValue != newValue) {
			// Push ParamChange history action
			history::ParamChange* h = new history::ParamChange;
			h->name = string::translate("ParamWidget.history.reset");
			h->moduleId = module->id;
			h->paramId = paramId;
			h->oldValue = oldValue;
			h->newValue = newValue;
			getHistory()->push(h);
		}
	}
}


} // namespace app
} // namespace rack
