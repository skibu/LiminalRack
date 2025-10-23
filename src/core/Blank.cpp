#include "plugin.hpp"
#include <context.hpp>


namespace rack {
namespace core {


struct BlankModule : Module {
	int width = 10;

	/** Legacy for <=v1 patches */
	void fromJson(json_t* rootJ) override {
		Module::fromJson(rootJ);
		json_t* widthJ = json_object_get(rootJ, "width");
		if (widthJ)
			width = std::round(json_number_value(widthJ) / RACK_GRID_WIDTH);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "width", json_integer(width));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* widthJ = json_object_get(rootJ, "width");
		if (widthJ)
			width = json_integer_value(widthJ);
	}
};


struct BlankPanel : Widget {
	Widget* panelBorder;

	BlankPanel() {
		panelBorder = new PanelBorder;
		addChild(panelBorder);
	}

	void step() override {
		panelBorder->setSize(getSize());
		Widget::step();
	}

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.0, 0.0, getWidth(), getHeight());
		NVGcolor bg = settings::preferDarkPanels ? nvgRGB(42, 42, 42) : nvgRGB(235, 235, 235);
		nvgFillColor(args.vg, bg);
		nvgFill(args.vg);
		Widget::draw(args);
	}
};


struct ModuleResizeHandle : OpaqueWidget {
	bool right = false;
	Vec dragPos;
	Rect originalBox;
	BlankModule* module;

	ModuleResizeHandle() {
		setSize(RACK_GRID_WIDTH * 1, RACK_GRID_HEIGHT);
	}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		dragPos = getRack()->getMousePos();
		ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
		assert(mw);
		originalBox = mw->getBox();
	}

	void onDragMove(const DragMoveEvent& e) override {
		ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
		assert(mw);

		Vec newDragPos = getRack()->getMousePos();
		float deltaX = newDragPos.getX() - dragPos.getX();

		Rect newBox = originalBox;
		Rect oldBox = mw->getBox();
		const float minWidth = 3 * RACK_GRID_WIDTH;
		if (right) {
			float targetWidth = newBox.getWidth() + deltaX;
			targetWidth = std::fmax(targetWidth, minWidth);
			targetWidth = std::round(targetWidth / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
			newBox.setWidth(targetWidth);
		}
		else {
			float targetWidth = newBox.getWidth() - deltaX;
			targetWidth = std::fmax(targetWidth, minWidth);
			targetWidth = std::round(targetWidth / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
			newBox.setWidth(targetWidth);
			float newPosX = originalBox.getPosX() + originalBox.getWidth() - targetWidth;
			newBox.setPosX(newPosX);
		}

		// Set box and test whether it's valid
		mw->setBox(newBox);
		if (!getRack()->requestModulePos(mw, newBox.getPos())) {
			mw->setBox(oldBox);
		}
		module->width = std::round(mw->getWidth() / RACK_GRID_WIDTH);
	}

	void draw(const DrawArgs& args) override {
		for (float x = 5.0; x <= 10.0; x += 5.0) {
			nvgBeginPath(args.vg);
			const float margin = 5.0;
			nvgMoveTo(args.vg, x + 0.5, margin + 0.5);
			nvgLineTo(args.vg, x + 0.5, getHeight() - margin + 0.5);
			nvgStrokeWidth(args.vg, 1.0);
			nvgStrokeColor(args.vg, nvgRGBAf(0.5, 0.5, 0.5, 0.5));
			nvgStroke(args.vg);
		}
	}
};


struct BlankWidget : ModuleWidget {
	Widget* topRightScrew;
	Widget* bottomRightScrew;
	Widget* rightHandle;
	BlankPanel* blankPanel;

	BlankWidget(BlankModule* module) {
		setModule(module);
		setSize(Vec(RACK_GRID_WIDTH * 10, RACK_GRID_HEIGHT));

		blankPanel = new BlankPanel;
		addChild(blankPanel);

		ModuleResizeHandle* leftHandle = new ModuleResizeHandle;
		leftHandle->module = module;
		addChild(leftHandle);

		ModuleResizeHandle* rightHandle = new ModuleResizeHandle;
		rightHandle->right = true;
		this->rightHandle = rightHandle;
		rightHandle->module = module;
		addChild(rightHandle);

		addChild(createWidget<ThemedScrew>(Vec(15, 0)));
		addChild(createWidget<ThemedScrew>(Vec(15, 365)));
		topRightScrew = createWidget<ThemedScrew>(Vec(getWidth() - 30, 0));
		bottomRightScrew = createWidget<ThemedScrew>(Vec(getWidth() - 30, 365));
		addChild(topRightScrew);
		addChild(bottomRightScrew);

		// Set box width from loaded Module before adding to the RackWidget, so modules aren't unnecessarily shoved around.
		if (module) {
			setWidth(module->width * RACK_GRID_WIDTH);
		}
	}

	void step() override {
		BlankModule* module = dynamic_cast<BlankModule*>(this->module);
		if (module) {
			setWidth(module->width * RACK_GRID_WIDTH);
		}

		blankPanel->setSize(getSize());
		topRightScrew->setX(getWidth() - 30);
		bottomRightScrew->setX(getWidth() - 30);
		if (getWidth() < RACK_GRID_WIDTH * 6) {
			topRightScrew->hide();
			bottomRightScrew->hide();
		}
		else {
			topRightScrew->show();
			bottomRightScrew->show();
		}
		rightHandle->setX(getWidth() - rightHandle->getWidth());
		ModuleWidget::step();
	}
};


Model* modelBlank = createModel<BlankModule, BlankWidget>("Blank");


} // namespace core
} // namespace rack
