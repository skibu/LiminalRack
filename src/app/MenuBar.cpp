#include <thread>
#include <utility>
#include <algorithm>

#include <osdialog.h>

#include <app/MenuBar.hpp>
#include <app/TipWindow.hpp>
#include <app/Browser.hpp>
#include <widget/OpaqueWidget.hpp>
#include <ui/Button.hpp>
#include <ui/MenuItem.hpp>
#include <ui/MenuSeparator.hpp>
#include <ui/SequentialLayout.hpp>
#include <ui/Slider.hpp>
#include <ui/TextField.hpp>
#include <ui/ProgressBar.hpp>
#include <ui/Label.hpp>
#include <engine/Engine.hpp>
#include <window/Window.hpp>
#include <asset.hpp>
#include <context.hpp>
#include <settings.hpp>
#include <helpers.hpp>
#include <string.hpp>
#include <system.hpp>
#include <plugin.hpp>
#include <patch.hpp>
#include <library.hpp>


namespace rack {
namespace app {
namespace menuBar {

/**
 * MenuButtons are buttons that are in the main menu bar. They are for pulling
 * down a menu. Each button subclasses from MenuButton in order to handle the
 * appropriate action items.
 */
class MenuButton : public ui::Button {
   public:
    MenuButton(const std::string& text) : ui::Button(text) {}

    // Handle actions
    void step() override {
        setWidth(bndLabelWidth(getWindow()->vg_, -1, text_.c_str()) + 1.0);
        Widget::step();
    }

    // Handle drawing the button
    void draw(const DrawArgs& args) override {
        // Determine state to draw button
        BNDwidgetState state = BND_DEFAULT;  // Normal look
        if (getEvent()->getHoveredWidget() == this)
            state = BND_HOVER;  // Mouse over button
        if (getEvent()->getDraggedWidget() == this)
            state = BND_ACTIVE;  // Clicked on and menu pulled down

        // Draw the button
		bndMenuItem(args.vg, 0.0, 0.0, getWidth(), getHeight(), state, -1,
					text_.c_str());

        // Draw all the nodes
        Widget::draw(args);
    }
};

/**
 * For drawing red dot in a menu bar button to indicate a notification
 */
struct NotificationIcon : widget::Widget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		float radius = 4;
		nvgCircle(args.vg, radius, radius, radius);
		nvgFillColor(args.vg, nvgRGBf(1.0, 0.0, 0.0));
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBf(0.5, 0.0, 0.0));
		nvgStroke(args.vg);
	}
};


////////////////////
// File
////////////////////

/**
 * The File button for the main menu
 */
class FileButton : public MenuButton {
   public:
    FileButton() : MenuButton(string::translate("MenuBar.file")) {}

   private:
    void onAction(const ActionEvent& e) override {
        DEBUG("FileButton::onAction() called");

        ui::Menu* menu = createMenu();
        menu->cornerFlags = BND_CORNER_TOP;
        menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));
        menu->addChild(new ui::MenuSeparator);

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.new"),
                           widget::getKeyCommandName(GLFW_KEY_N, RACK_MOD_CTRL),
                           []() { getPatch()->loadTemplateDialog(); }));

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.open"),
                           widget::getKeyCommandName(GLFW_KEY_O, RACK_MOD_CTRL),
                           []() { getPatch()->loadDialog(); }));

        menu->addChild(createSubmenuItem(
            string::translate("MenuBar.file.openRecent"), "",
            [](ui::Menu* menu) {
                for (const std::string& path : settings::recentPatchPaths) {
                    std::string name = system::getStem(path);
                    menu->addChild(createMenuItem(
                        name, "", [=]() { getPatch()->loadPathDialog(path); }));
                }
            },
            settings::recentPatchPaths.empty()));

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.save"),
                           widget::getKeyCommandName(GLFW_KEY_S, RACK_MOD_CTRL),
                           []() { 
                            DEBUG("FileButton::onAction() calling getPatch()->saveDialog()");
                            getPatch()->saveDialog(); 
                        }));

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.saveAs"),
                           widget::getKeyCommandName(
                               GLFW_KEY_S, RACK_MOD_CTRL | GLFW_MOD_SHIFT),
                           []() { getPatch()->saveAsDialog(); }));

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.saveCopy"), "",
                           []() { getPatch()->saveAsDialog(false); }));

        menu->addChild(createMenuItem(
            string::translate("MenuBar.file.revert"),
            widget::getKeyCommandName(GLFW_KEY_O,
                                      RACK_MOD_CTRL | GLFW_MOD_SHIFT),
            []() { getPatch()->revertDialog(); }, getPatch()->path == ""));

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.overwriteTemplate"),
                           "", []() { getPatch()->saveTemplateDialog(); }));

        menu->addChild(new ui::MenuSeparator);

        // Load selection
        menu->addChild(createMenuItem(
            string::translate("MenuBar.file.importSelection"), "",
            [=]() { getRack()->loadSelectionDialog(); }, false, true));

        menu->addChild(new ui::MenuSeparator);

        menu->addChild(
            createMenuItem(string::translate("MenuBar.file.quit"),
                           widget::getKeyCommandName(GLFW_KEY_Q, RACK_MOD_CTRL),
                           []() { getWindow()->close(); }));
    }
};

////////////////////
// Edit
////////////////////


/**
 * The Edit button for the main menu
 */
class EditButton : public MenuButton {
   public:
    EditButton() : MenuButton(string::translate("MenuBar.edit")) {}

   private:
    void onAction(const ActionEvent& e) override {
        DEBUG("EditButton::onAction called");

        ui::Menu* menu = createMenu();
        menu->cornerFlags = BND_CORNER_TOP;
        menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));

		menu->addChild(new ui::MenuSeparator);

		class UndoItem : public ui::MenuItem {
            public:
            UndoItem() {}

            private:
			void step() override {
				bool canUndo = getHistory()->canUndo();
				setText(canUndo ? string::f(string::translate("MenuBar.edit.undoAction"), getHistory()->getUndoName()) : string::translate("MenuBar.edit.undo"));
				setDisabled(!canUndo);
				MenuItem::step();
			}
			void onAction(const ActionEvent& e) override {
                DEBUG("UndoItem::onAction called");
				getHistory()->undo();
			}
		};
		menu->addChild(createMenuItem<UndoItem>("", widget::getKeyCommandName(GLFW_KEY_Z, RACK_MOD_CTRL)));

		class RedoItem : public ui::MenuItem {
            public:
            RedoItem() {}

            private:
			void step() override {
				bool canRedo = getHistory()->canRedo();
				setText(canRedo ? string::f(string::translate("MenuBar.edit.redoAction"), getHistory()->getRedoName()) : string::translate("MenuBar.edit.redo"));
				setDisabled(!canRedo);
				MenuItem::step();
			}
			void onAction(const ActionEvent& e) override {
				getHistory()->redo();
			}
		};
		menu->addChild(createMenuItem<RedoItem>("", widget::getKeyCommandName(GLFW_KEY_Z, RACK_MOD_CTRL | GLFW_MOD_SHIFT)));

		menu->addChild(createMenuItem(string::translate("MenuBar.edit.clearCables"), "", [=]() {
			getPatch()->disconnectDialog();
		}));

		menu->addChild(new ui::MenuSeparator);

		// Add button for adding a module by opening up the local module browser
		menu->addChild(createMenuItem(string::translate("MenuBar.library.addModuleToRack"), "", [=]() {
			getScene()->getBrowserOverlay()->show();
		}));

		// Add select all modules button
    	menu->addChild(createMenuItem(
        string::translate("RackWidget.selectAll"),
        widget::getKeyCommandName(GLFW_KEY_A, RACK_MOD_CTRL), [=]() { getRack()->selectAll(); }, false, true));

		// Add module related menu items
		menu->addChild(new ui::MenuSeparator);
		menu->addChild(createMenuLabel(string::translate("MenuBar.edit.moduleContextMenuHeader")));

		// Append context menu for the module so user can affect it
		getRack()->appendSelectionContextMenu(menu);
	}
};


////////////////////
// View
////////////////////

class ZoomQuantity : public Quantity {
   public:
    void setValue(float value) override {
        getScene()->getRackScroll()->setZoom(std::pow(2.f, value));
    }
    float getValue() override {
        return std::log2(getScene()->getRackScroll()->getZoom());
    }
    float getMinValue() override {
        return -2.f;
    }
    float getMaxValue() override {
        return 2.f;
    }
    float getDefaultValue() override {
        return 0.0;
    }
    float getDisplayValue() override {
        return std::round(std::pow(2.f, getValue()) * 100);
    }
    void setDisplayValue(float displayValue) override {
        setValue(std::log2(displayValue / 100));
    }
    std::string getLabel() override {
        return string::translate("MenuBar.view.zoom");
    }
    std::string getUnit() override {
        return "%";
    }
};

class ZoomSlider : public ui::Slider {
   public:
   /** Construct a slider with a zoom quantity */
    ZoomSlider() : ui::Slider(new ZoomQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~ZoomSlider() {
        delete quantity;
    }
};

class CableOpacityQuantity : public Quantity {
   public:
    void setValue(float value) override {
        settings::cableOpacity =
            math::clamp(value, getMinValue(), getMaxValue());
    }
    float getValue() override {
        return settings::cableOpacity;
    }
    float getDefaultValue() override {
        return 0.5;
    }
    float getDisplayValue() override {
        return getValue() * 100;
    }
    void setDisplayValue(float displayValue) override {
        setValue(displayValue / 100);
    }
    std::string getLabel() override {
        return string::translate("MenuBar.view.cableOpacity");
    }
    std::string getUnit() override {
        return "%";
    }
};

class CableOpacitySlider : public ui::Slider {
   public:
   /** Construct a slider with a cable opacity quantity */
    CableOpacitySlider() : ui::Slider(new CableOpacityQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~CableOpacitySlider() {
        delete quantity;
    }
};

class CableTensionQuantity : public Quantity {
   public:
	void setValue(float value) override {
		settings::cableTension = math::clamp(value, getMinValue(), getMaxValue());
	}
	float getValue() override {
		return settings::cableTension;
	}
	float getDefaultValue() override {
		return 0.5;
	}
	float getDisplayValue() override {
		return getValue() * 100;
	}
	void setDisplayValue(float displayValue) override {
		setValue(displayValue / 100);
	}
	std::string getLabel() override {
		return string::translate("MenuBar.view.cableTension");
	}
	std::string getUnit() override {
		return "%";
	}
};

class CableTensionSlider : public ui::Slider {
   public:
    /** Construct a slider with a cable tension quantity */
    CableTensionSlider() : ui::Slider(new CableTensionQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~CableTensionSlider() {
        delete quantity;
    }
};

class CableTensionRandomFactorQuantity : public Quantity {
   public:
    void setValue(float value) override {
        settings::cableTensionRandomFactor =
            math::clamp(value, getMinValue(), getMaxValue());
    }
        float getValue() override {
		return settings::cableTensionRandomFactor;
	}
	float getDefaultValue() override {
		return 0.1f;
	}
    float getMinValue() override {
        return 0.0f;
    }
    float getMaxValue() override {
        return 0.3f;
    }
	float getDisplayValue() override {
		return getValue() * 100;
	}
	void setDisplayValue(float displayValue) override {
		setValue(displayValue / 100);
	}
	std::string getLabel() override {
		return string::translate("MenuBar.view.cableTensionRandomFactor");
	}
	std::string getUnit() override {
		return "%";
	}
};

class CableTensionRandomFactorSlider : public ui::Slider {
   public:
    /** Construct a slider with a cable tension random quantity */
    CableTensionRandomFactorSlider()
        : ui::Slider(new CableTensionRandomFactorQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~CableTensionRandomFactorSlider() {
        delete quantity;
    }
};

class RackBrightnessQuantity : public Quantity {
   public:
	void setValue(float value) override {
		settings::rackBrightness = math::clamp(value, getMinValue(), getMaxValue());
	}
	float getValue() override {
		return settings::rackBrightness;
	}
	float getDefaultValue() override {
		return 1.0;
	}
	float getDisplayValue() override {
		return getValue() * 100;
	}
	void setDisplayValue(float displayValue) override {
		setValue(displayValue / 100);
	}
	std::string getUnit() override {
		return "%";
	}
	std::string getLabel() override {
		return string::translate("MenuBar.view.roomBrightness");
	}
};

class RackBrightnessSlider : public ui::Slider {
   public:
    /** Construct a slider with a rack brightness quantity */
    RackBrightnessSlider() : ui::Slider(new RackBrightnessQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~RackBrightnessSlider() {
        delete quantity;
    }
};

class HaloBrightnessQuantity : public Quantity {
    public:
	void setValue(float value) override {
		settings::haloBrightness = math::clamp(value, getMinValue(), getMaxValue());
	}
	float getValue() override {
		return settings::haloBrightness;
	}
	float getDefaultValue() override {
		return 0.25;
	}
	float getDisplayValue() override {
		return getValue() * 100;
	}
	void setDisplayValue(float displayValue) override {
		setValue(displayValue / 100);
	}
	std::string getUnit() override {
		return "%";
	}
	std::string getLabel() override {
		return string::translate("MenuBar.view.lightBloom");
	}
};

class HaloBrightnessSlider : public ui::Slider {
   public:
    /** Construct a slider with a halo brightness quantity */
    HaloBrightnessSlider() : ui::Slider(new HaloBrightnessQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~HaloBrightnessSlider() {
        delete quantity;
    }
};

class KnobScrollSensitivityQuantity : public Quantity {
   public:
    void setValue(float value) override {
        value = math::clamp(value, getMinValue(), getMaxValue());
        settings::knobScrollSensitivity = std::pow(2.f, value);
    }
    float getValue() override {
        return std::log2(settings::knobScrollSensitivity);
    }
    float getMinValue() override {
        return std::log2(1e-4f);
    }
    float getMaxValue() override {
        return std::log2(1e-2f);
    }
    float getDefaultValue() override {
        return std::log2(1e-3f);
    }
    float getDisplayValue() override {
        return std::pow(2.f, getValue() - getDefaultValue());
    }
    void setDisplayValue(float displayValue) override {
        setValue(std::log2(displayValue) + getDefaultValue());
    }
    std::string getLabel() override {
        return string::translate("MenuBar.view.wheelSensitivity");
    }
    int getDisplayPrecision() override {
        return 2;
    }
};

class KnobScrollSensitivitySlider : public ui::Slider {
   public:
    /** Construct a slider with a knob scroll sensitivity quantity */
    KnobScrollSensitivitySlider()
        : ui::Slider(new KnobScrollSensitivityQuantity()) {}

    /** Since API requires passing in pointer, need to manually delete what we
     * created and passed in  */
    ~KnobScrollSensitivitySlider() {
        delete quantity;
    }
};

/**
 * The View button for the main menu
 */
class ViewButton : public MenuButton {
   public:
    ViewButton() : MenuButton(string::translate("MenuBar.view")) {}

    // Called when popping up View menu
    void onAction(const ActionEvent& e) override {
        ui::Menu* menu = createMenu();
        menu->cornerFlags = BND_CORNER_TOP;
        menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));

        // Add Window category menu label (inactive)
        menu->addChild(new ui::MenuSeparator);
        menu->addChild(
            createMenuLabel(string::translate("MenuBar.view.window")));

        // Add fullscreen menu item
        bool fullscreen = getWindow()->isFullScreen();
        std::string fullscreenText = widget::getKeyCommandName(GLFW_KEY_F11, 0);
        if (fullscreen) fullscreenText += " " CHECKMARK_STRING;
        menu->addChild(createMenuItem(
            string::translate("MenuBar.view.fullscreen"), fullscreenText,
            [=]() { getWindow()->setFullScreen(!fullscreen); }));

        // Only provide frame rate option if not VCV rack because it is a
        // obscure feature
        if (!settings::isNotVCVRack) {
            menu->addChild(createSubmenuItem(
                string::translate("MenuBar.view.frameRate"),
                string::f("%.0f Hz", settings::frameRateLimit),
                [=](ui::Menu* menu) {
                    for (int i = 1; i <= 6; i++) {
                        double frameRate =
                            getWindow()->getMonitorRefreshRate() / i;
                        menu->addChild(createCheckMenuItem(
                            string::f("%.0f Hz", frameRate), "",
                            [=]() {
                                return settings::frameRateLimit == frameRate;
                            },
                            [=]() { settings::frameRateLimit = frameRate; }));
                    }
                }));
        }

        // Only provide pixel ratio option if not VCV rack because it is a
        // obscure feature
        if (!settings::isNotVCVRack) {
            static const std::vector<float> pixelRatios = {0, 1,   1.5,
                                                           2, 2.5, 3};
            std::vector<std::string> pixelRatioLabels;
            for (float pixelRatio : pixelRatios) {
                pixelRatioLabels.push_back(
                    pixelRatio == 0.f
                        ? string::translate("MenuBar.view.pixelRatio.auto")
                        : string::f("%0.f%%", pixelRatio * 100.f));
            }
            menu->addChild(createIndexSubmenuItem(
                string::translate("MenuBar.view.pixelRatio"), pixelRatioLabels,
                [=]() -> size_t {
                    auto it = std::find(pixelRatios.begin(), pixelRatios.end(),
                                        settings::pixelRatio);
                    if (it == pixelRatios.end()) return -1;
                    return it - pixelRatios.begin();
                },
                [=](size_t i) { settings::pixelRatio = pixelRatios[i]; }));
        }

        // Add zoom slider
        ZoomSlider* zoomSlider = new ZoomSlider;
        zoomSlider->setWidth(250.0);
        menu->addChild(zoomSlider);

        // Add menu button to zoom fit to modules
        menu->addChild(createMenuItem(
            string::translate("MenuBar.view.zoomFit"),
            widget::getKeyCommandName(GLFW_KEY_F4, 0),
            [=]() { getScene()->getRackScroll()->zoomToModules(); }));

        // Create zoom sub menu, if not in Liminal mode
        if (!settings::isNotVCVRack) {
            menu->addChild(createIndexPtrSubmenuItem(
                string::translate("MenuBar.view.mouseWheelZoom"),
                {string::f(
                     string::translate("MenuBar.view.mouseWheelZoom.scroll"),
                     RACK_MOD_CTRL_NAME),
                 string::f(
                     string::translate("MenuBar.view.mouseWheelZoom.zoom"),
                     RACK_MOD_CTRL_NAME)},
                &settings::mouseWheelZoom));
        }

        // Add Appearance category menu label (inactive)
        menu->addChild(new ui::MenuSeparator);
        menu->addChild(
            createMenuLabel(string::translate("MenuBar.view.appearance")));

        if (!settings::isNotVCVRack) {
            static const std::vector<std::string> uiThemes = {"dark", "light",
                                                              "hcdark"};
            menu->addChild(createIndexSubmenuItem(
                string::translate("MenuBar.view.uiTheme"),
                {string::translate("MenuBar.view.appearance.dark"),
                 string::translate("MenuBar.view.appearance.light"),
                 string::translate("MenuBar.view.appearance.hcdark")},
                [=]() -> size_t {
                    auto it = std::find(uiThemes.begin(), uiThemes.end(),
                                        settings::uiTheme);
                    if (it == uiThemes.end()) return -1;
                    return it - uiThemes.begin();
                },
                [=](size_t i) {
                    settings::uiTheme = uiThemes[i];
                    ui::refreshTheme();
                }));
        }

        menu->addChild(createBoolPtrMenuItem(
            string::translate("MenuBar.view.showTooltips"), "",
            &settings::tooltips));

        // Various sliders
        CableOpacitySlider* cableOpacitySlider = new CableOpacitySlider();
        cableOpacitySlider->setWidth(250.0);
        menu->addChild(cableOpacitySlider);

        CableTensionSlider* cableTensionSlider = new CableTensionSlider();
        cableTensionSlider->setWidth(250.0);
        menu->addChild(cableTensionSlider);

        CableTensionRandomFactorSlider* cableTensionRandomFactorSlider =
            new CableTensionRandomFactorSlider();
        cableTensionRandomFactorSlider->setWidth(250.0);
        menu->addChild(cableTensionRandomFactorSlider);

        RackBrightnessSlider* rackBrightnessSlider = new RackBrightnessSlider();
        rackBrightnessSlider->setWidth(250.0);
        menu->addChild(rackBrightnessSlider);

        HaloBrightnessSlider* haloBrightnessSlider = new HaloBrightnessSlider();
        haloBrightnessSlider->setWidth(250.0);
        menu->addChild(haloBrightnessSlider);

        menu->addChild(createBoolPtrMenuItem(
            string::translate("MenuBar.view.showKnobShadows"), "",
            &settings::showKnobShadows));

        // Cable colors
        menu->addChild(createSubmenuItem(
            string::translate("MenuBar.view.cableColors"), "",
            [=](ui::Menu* menu) {
                // TODO Subclass Menu to make an auto-refreshing list so user
                // can Ctrl+click to keep menu open.

                // Add color items
                for (size_t i = 0; i < settings::cableColors.size(); i++) {
                    NVGcolor color = settings::cableColors[i];
                    std::string label = get(settings::cableLabels, i);
                    std::string labelFallback =
                        (label != "")
                            ? label
                            : string::f("Color #%lld", (long long)(i + 1));

                    ui::ColorDotMenuItem* item = createSubmenuItem<
                        ui::ColorDotMenuItem>(
                        labelFallback, "", [=](ui::Menu* menu) {
                            // Helper for launching color dialog
                            auto selectColor = [](NVGcolor& color) -> bool {
                                osdialog_color c = {
                                    uint8_t(color.r * 255.f),
                                    uint8_t(color.g * 255.f),
                                    uint8_t(color.b * 255.f),
                                    uint8_t(color.a * 255.f),
                                };
                                if (!osdialog_color_picker(&c, false))
                                    return false;
                                color = nvgRGBA(c.r, c.g, c.b, c.a);
                                return true;
                            };

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.setLabel"),
                                "",
                                [=]() {
                                    if (i >= settings::cableColors.size())
                                        return;
                                    char* s = osdialog_prompt(OSDIALOG_INFO, "",
                                                              label.c_str());
                                    if (!s) return;
                                    settings::cableLabels.resize(
                                        settings::cableColors.size());
                                    settings::cableLabels[i] = s;
                                    free(s);
                                },
                                false, true));

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.setColor"),
                                "",
                                [=]() {
                                    if (i >= settings::cableColors.size())
                                        return;
                                    NVGcolor newColor = color;
                                    if (!selectColor(newColor)) return;
                                    std::memcpy(&settings::cableColors[i],
                                                &newColor, sizeof(newColor));
                                },
                                false, true));

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.newColorAbove"),
                                "",
                                [=]() {
                                    if (i >= settings::cableColors.size())
                                        return;
                                    NVGcolor newColor = color;
                                    if (!selectColor(newColor)) return;
                                    settings::cableLabels.resize(
                                        settings::cableColors.size());
                                    settings::cableColors.insert(
                                        settings::cableColors.begin() + i,
                                        newColor);
                                    settings::cableLabels.insert(
                                        settings::cableLabels.begin() + i, "");
                                },
                                false, true));

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.newColorBelow"),
                                "",
                                [=]() {
                                    if (i >= settings::cableColors.size())
                                        return;
                                    NVGcolor newColor = color;
                                    if (!selectColor(newColor)) return;
                                    settings::cableLabels.resize(
                                        settings::cableColors.size());
                                    settings::cableColors.insert(
                                        settings::cableColors.begin() + i + 1,
                                        newColor);
                                    settings::cableLabels.insert(
                                        settings::cableLabels.begin() + i + 1,
                                        "");
                                },
                                false, true));

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.moveUp"),
                                "",
                                [=]() {
                                    if (i < 1 ||
                                        i >= settings::cableColors.size())
                                        return;
                                    settings::cableLabels.resize(
                                        settings::cableColors.size());
                                    std::swap(settings::cableColors[i],
                                              settings::cableColors[i - 1]);
                                    std::swap(settings::cableLabels[i],
                                              settings::cableLabels[i - 1]);
                                },
                                i < 1, true));

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.moveDown"),
                                "",
                                [=]() {
                                    if (i + 1 >= settings::cableColors.size())
                                        return;
                                    settings::cableLabels.resize(
                                        settings::cableColors.size());
                                    std::swap(settings::cableColors[i],
                                              settings::cableColors[i + 1]);
                                    std::swap(settings::cableLabels[i],
                                              settings::cableLabels[i + 1]);
                                },
                                i + 1 >= settings::cableColors.size()));

                            menu->addChild(createMenuItem(
                                string::translate(
                                    "MenuBar.view.cableColors.delete"),
                                "",
                                [=]() {
                                    if (i >= settings::cableColors.size())
                                        return;
                                    settings::cableLabels.resize(
                                        settings::cableColors.size());
                                    settings::cableColors.erase(
                                        settings::cableColors.begin() + i);
                                    settings::cableLabels.erase(
                                        settings::cableLabels.begin() + i);
                                },
                                settings::cableColors.size() <= 1, true));
                        });
                    item->color = color;
                    menu->addChild(item);
                }

                // Don't need to autorotate or restore cable colors in Liminal
                if (!settings::isNotVCVRack) {
                    menu->addChild(createBoolMenuItem(
                        string::translate(
                            "MenuBar.view.cableColors.autoRotate"),
                        "", [=]() -> bool { return settings::cableAutoRotate; },
                        [=](bool s) { settings::cableAutoRotate = s; }));
                    menu->addChild(createMenuItem(
                        string::translate(
                            "MenuBar.view.cableColors.restoreFactory"),
                        "",
                        [=]() {
                            if (!osdialog_message(
                                    OSDIALOG_WARNING, OSDIALOG_OK_CANCEL,
                                    string::translate(
                                        "MenuBar.view.cableColors."
                                        "overwriteFactory")
                                        .c_str()))
                                return;
                            settings::resetCables();
                        },
                        false, true));
                }
            }));

        // Add Parameters category menu label (inactive)
        menu->addChild(new ui::MenuSeparator);
        menu->addChild(
            createMenuLabel(string::translate("MenuBar.view.parameters")));

        // Usually want to hide cursor when turning a knob so don't need to make
        // this settable. But if VCVRack best to not change the UI.
        if (!settings::isNotVCVRack) {
            menu->addChild(createBoolPtrMenuItem(
                string::translate("MenuBar.view.lockCursor"), "",
                &settings::allowCursorLock));
        }

        static const std::vector<std::string> knobModeLabels = {
            string::translate("MenuBar.view.knob.linear"),
            string::translate("MenuBar.view.knob.scaledLinear"),
            string::translate("MenuBar.view.knob.absRotary"),
            string::translate("MenuBar.view.knob.relRotary"),
        };
        static const std::vector<int> knobModes = {0, 2, 3};
        menu->addChild(createSubmenuItem(
            string::translate("MenuBar.view.knob"),
            knobModeLabels[settings::knobMode], [=](ui::Menu* menu) {
                for (int knobMode : knobModes) {
                    menu->addChild(createCheckMenuItem(
                        knobModeLabels[knobMode], "",
                        [=]() { return settings::knobMode == knobMode; },
                        [=]() {
                            settings::knobMode = (settings::KnobMode)knobMode;
                        }));
                }
            }));

        if (!settings::isNotVCVRack) {
            menu->addChild(createBoolPtrMenuItem(
                string::translate("MenuBar.view.knobScroll"), "",
                &settings::knobScroll));

            KnobScrollSensitivitySlider* knobScrollSensitivitySlider =
                new KnobScrollSensitivitySlider();
            knobScrollSensitivitySlider->setWidth(250.0);
            menu->addChild(knobScrollSensitivitySlider);
        }

        // Add Modules category menu label (inactive)
        menu->addChild(new ui::MenuSeparator);
        menu->addChild(
            createMenuLabel(string::translate("MenuBar.view.modules")));

        menu->addChild(
            createBoolPtrMenuItem(string::translate("MenuBar.view.lockModules"),
                                  "", &settings::lockModules));
        if (settings::isNotVCVRack) {
            // Nice to be able to add modules within the View Modules
            // section
            menu->addChild(createMenuItem(
                string::translate("MenuBar.library.addModuleToRack"), "",
                [=]() { getScene()->getBrowserOverlay()->show(); }));
        } else {
            // These options not that useful so removed when not VCVRack
            // but left in otherwise to keep the VCV Rack UI consistent
            menu->addChild(createBoolPtrMenuItem(
                string::translate("MenuBar.view.squeezeModules"), "",
                &settings::squeezeModules));
        }

        // Allow user to prefer dark or light colored panels. Some
        // people probably picky.
        menu->addChild(createBoolPtrMenuItem(
            string::translate("MenuBar.view.preferDarkPanels"), "",
            &settings::preferDarkPanels));
    }
};  // End of class ViewButton

////////////////////
// Engine
////////////////////

struct SampleRateItem : ui::MenuItem {
    ui::Menu* createChildMenu() override {
        ui::Menu* menu = new ui::Menu;

        // Auto sample rate
        std::string rightText;
        if (settings::sampleRate == 0) {
            float sampleRate = getEngine()->getSampleRate();
            rightText += string::f("(%g kHz) ", sampleRate / 1000.f);
        }
        menu->addChild(createCheckMenuItem(
            string::translate("MenuBar.engine.sampleRate.auto"), rightText,
            [=]() { return settings::sampleRate == 0; },
            [=]() { settings::sampleRate = 0; }));

        // Power-of-2 oversample times 44.1kHz or 48kHz
        for (int i = -1; i <= 2; i++) {
            // Originally would do both relative to 44.1kHz and 48kHz, but
            // this is too many options.
            int minj = i == 0 ? 0 : 1;
            int maxj = 2;
            for (int j = minj; j < maxj; j++) {
                float oversample = std::pow(2.f, i);
                float sampleRate = (j == 0) ? 44100.f : 48000.f;
                sampleRate *= oversample;

                std::string text = string::f("%g kHz", sampleRate / 1000.f);
                std::string rightText;
                if (oversample > 1.f) {
                    rightText += string::f("(%.0fx)", oversample);
                } else if (oversample < 1.f) {
                    rightText += string::f("(1/%.0fx)", 1.f / oversample);
                }
                menu->addChild(createCheckMenuItem(
                    text, rightText,
                    [=]() { return settings::sampleRate == sampleRate; },
                    [=]() { settings::sampleRate = sampleRate; }));
            }
        }
        return menu;
    }
};

/**
 * The Engine button for the main menu
 */
class EngineButton : public MenuButton {
    public:
    EngineButton() : MenuButton(string::translate("MenuBar.engine")) {}

    private:
    void onAction(const ActionEvent& e) override {
        ui::Menu* menu = createMenu();
        menu->cornerFlags = BND_CORNER_TOP;
        menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));

        menu->addChild(new ui::MenuSeparator);

        std::string cpuMeterText =
            widget::getKeyCommandName(GLFW_KEY_F3, 0);
        if (settings::cpuMeter) cpuMeterText += " " CHECKMARK_STRING;
        menu->addChild(createMenuItem(
            string::translate("MenuBar.engine.cpuMeter"), cpuMeterText,
            [=]() { settings::cpuMeter ^= true; }));

        menu->addChild(createMenuItem<SampleRateItem>(
            string::translate("MenuBar.engine.sampleRate"), RIGHT_ARROW));

        if (!settings::isNotVCVRack) {
            menu->addChild(createSubmenuItem(
                string::translate("MenuBar.engine.threads"),
                string::f("%d", settings::threadCount),
                [=](ui::Menu* menu) {
                    // BUG This assumes SMT is enabled.
                    int cores = system::getLogicalCoreCount() / 2;

                    for (int i = 1; i <= 2 * cores; i++) {
                        std::string rightText;
                        if (i == cores)
                            rightText += string::translate(
                                "MenuBar.engine.threads.most");
                        else if (i == 1)
                            rightText += string::translate(
                                "MenuBar.engine.threads.lowest");
                        menu->addChild(createCheckMenuItem(
                            string::f("%d", i), rightText,
                            [=]() { return settings::threadCount == i; },
                            [=]() { settings::threadCount = i; }));
                    }
                }));
        }
    }
};

////////////////////
// Plugins
////////////////////

class AccountPasswordField : public ui::PasswordField {
    public:
    AccountPasswordField() {}

    void setLogInItem(ui::MenuItem* item) {
        logInItem = item;
    }

    private:
    ui::MenuItem* logInItem;

    void onAction(const ActionEvent& e) override {
        logInItem->doAction();
    }
};

/** A menu item button for actually logging in */
struct LogInItem : ui::MenuItem {
	ui::TextField* emailField;
	ui::TextField* passwordField;

    /** Called when user hits menu login button. Logs in the user
     * via a separate thread so as to not block the UI. */
	void onAction(const ActionEvent& e) override {
		std::string email = emailField->getText();
		std::string password = passwordField->getText();
		std::thread t([=] {
			library::logIn(email, password);
            if (library::isLoggedIn()) {
                // Close the login menu since login was successful
                DEBUG("Login successful, closing login menu");
                auto menu = getParent();
                menu->hide();
            }

			library::checkUpdates();
		});
		t.detach();
		e.unconsume();
	}

	void step() override {
		setText(string::translate("MenuBar.library.login"));
		setRightText(library::getLoginStatus());
		MenuItem::step();
	}
};


struct SyncUpdatesItem : ui::MenuItem {
	void step() override {
		if (library::getUpdateStatus() != "") {
			setText(library::getUpdateStatus());
		}
		else if (library::isSyncing()) {
			setText(string::translate("MenuBar.library.updating"));
		}
		else if (!library::hasUpdates()) {
			setText(string::translate("MenuBar.library.upToDate"));
		}
		else {
			setText(string::translate("MenuBar.library.updateAll"));
		}

		setDisabled(library::isSyncing() || !library::hasUpdates());
		MenuItem::step();
	}

	void onAction(const ActionEvent& e) override {
		std::thread t([=] {
			library::syncUpdates();
		});
		t.detach();
		e.unconsume();
	}
};


struct SyncUpdateItem : ui::MenuItem {
	std::string slug;

	void setUpdate(const std::string& slug) {
		this->slug = slug;

        auto updateInfos = library::getUpdateInfos();
		auto it = updateInfos.find(slug);
		if (it == updateInfos.end())
			return;
		library::UpdateInfo update = it->second;

		setText(update.name);
	}

	ui::Menu* createChildMenu() override {
        auto updateInfos = library::getUpdateInfos();

		auto it = updateInfos.find(slug);
		if (it == updateInfos.end())
			return NULL;
		library::UpdateInfo update = it->second;

		ui::Menu* menu = new ui::Menu;

		if (update.minRackVersion != "") {
			menu->addChild(createMenuLabel(string::f(string::translate("MenuBar.library.requiresRack"), update.minRackVersion)));
		}

		if (update.changelogUrl != "") {
			std::string changelogUrl = update.changelogUrl;
			menu->addChild(createMenuItem(string::translate("MenuBar.library.changelog"), "", [=]() {
				system::openBrowser(changelogUrl);
			}));
		}

		if (menu->getChildren().empty()) {
			delete menu;
			return NULL;
		}
		return menu;
	}

	void step() override {
		bool isDisabled = false;

		if (library::isSyncing())
			isDisabled = true;

        auto updateInfos = library::getUpdateInfos();
		auto it = updateInfos.find(slug);
		if (it == updateInfos.end()) {
			isDisabled = true;
		}
		else {
			library::UpdateInfo update = it->second;

			if (update.minRackVersion != "")
				isDisabled = true;

			if (update.downloaded) {
				setRightText(CHECKMARK_STRING);
				isDisabled = true;
			}
			else if (slug == library::getUpdateSlug()) {
				setRightText(string::f("%.0f%%", library::getUpdateProgress() * 100.f));
			}
			else {
				std::string rt = "";
				plugin::Plugin* p = plugin::getPlugin(slug);
				if (p) {
					rt += p->version + " → ";
				}
				rt += update.version;
				setRightText(rt);
			}
		}

		setDisabled(isDisabled);

		MenuItem::step();
	}

	void onAction(const ActionEvent& e) override {
		std::thread t([=] {
			library::syncUpdate(slug);
		});
		t.detach();
		e.unconsume();
	}
};


struct LibraryMenu : ui::Menu {
	LibraryMenu() {
		refresh();
	}

	void step() override {
		// Refresh menu when appropriate
		if (library::isRestartRequested()) {
			library::clearRestartRequest();
			refresh();
		}
		Menu::step();
	}

	void refresh() {
		setChildMenu(NULL);
		clearChildren();

		addChild(new ui::MenuSeparator);

		if (settings::devMode) {
			addChild(createMenuLabel(string::translate("MenuBar.library.devMode")));
		}

		// If user not logged in to VCV then they need to log in first
		else if (!library::isLoggedIn()) {
            // Create menu item for registering a new account on VCV website
			addChild(createMenuItem(string::translate("MenuBar.library.register"), "", [=]() {
				system::openBrowser("https://vcvrack.com/login");
			}));

            // Create separator before email and password fields
            addChild(new ui::MenuSeparator);
            addChild(createMenuLabel(string::translate("MenuBar.library.loginHeader")));

            // Create email field
			ui::TextField* emailField = new ui::TextField;
			emailField->setPlaceholder(string::translate("MenuBar.library.email"));
			emailField->setWidth(390.0);
			addChild(emailField);

            // Create password field
			AccountPasswordField* passwordField = new AccountPasswordField();
			passwordField->setPlaceholder(string::translate("MenuBar.library.password"));
			passwordField->setWidth(390.0);
			passwordField->setNextField(emailField);
			emailField->setNextField(passwordField);
			addChild(passwordField);

            // Create menu item button for actually logging in
			LogInItem* logInItem = new LogInItem;
			logInItem->emailField = emailField;
			logInItem->passwordField = passwordField;
			passwordField->setLogInItem(logInItem);
			addChild(logInItem);
		}
		// The regular module library options for when user is logged in
		else {
			addChild(createMenuItem(string::translate("MenuBar.library.addModuleToRack"), "", [=]() {
				getScene()->getBrowserOverlay()->show();
			}));

			addChild(new ui::MenuSeparator);

			addChild(createMenuItem(string::translate("MenuBar.library.browse"), "", [=]() {
				system::openBrowser("https://library.vcvrack.com/");
			}));

			addChild(createMenuItem(string::translate("MenuBar.library.account"), "", [=]() {
				system::openBrowser("https://vcvrack.com/account");
			}));

			addChild(createMenuItem(string::translate("MenuBar.library.logOut"), "", [=]() {
				library::logOut();
			}));

			addChild(new ui::MenuSeparator);

			// Add menu item to sync updates from VCV Rack library
			SyncUpdatesItem* syncItem = new SyncUpdatesItem;
			syncItem->setText(string::translate("MenuBar.library.updateAll"));
			addChild(syncItem);

			// Add buttons for updating individual collections of modules
			if (!library::getUpdateInfos().empty()) {
				addChild(new ui::MenuSeparator);
				addChild(createMenuLabel(string::translate("MenuBar.library.updates")));

				for (auto& pair : library::getUpdateInfos()) {
					SyncUpdateItem* updateItem = new SyncUpdateItem;
					updateItem->setUpdate(pair.first);
					addChild(updateItem);
				}
			}
		}
	}
};


/**
 * The Library button for the main menu
 */
class LibraryButton : public MenuButton {
   public:
	LibraryButton() : MenuButton(string::translate("MenuBar.library")) {
		notification = new NotificationIcon;
		addChild(notification);
	}

   private:
	// For drawing red dot in the Library Button for when there is a notification
	NotificationIcon* notification;

	void onAction(const ActionEvent& e) override {
		ui::Menu* menu = createMenu<LibraryMenu>();
		menu->cornerFlags = BND_CORNER_TOP;
		menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));

		menu->addChild(new ui::MenuSeparator);

		// Check for updates when menu is opened
		if (!settings::devMode) {
			std::thread t([&]() {
				system::setThreadName(string::translate("MenuBar.library"));
				library::checkUpdates();
			});
			t.detach();
		}
	}

	void step() override {
		notification->setPos(math::Vec(0, 0));
		notification->setVisible(library::hasUpdates());

		// Popup when updates finish downloading
		if (library::isRestartRequested()) {
			library::clearRestartRequest();
			if (osdialog_message(OSDIALOG_INFO, OSDIALOG_OK_CANCEL, string::translate("MenuBar.library.restart").c_str())) {
				getWindow()->close();
				settings::restart = true;
			}
		}

		MenuButton::step();
	}
};


////////////////////
// Help
////////////////////

/**
 * The Help button for the main menu
 */
class HelpButton : public MenuButton {
   public:
    HelpButton() : MenuButton(string::translate("MenuBar.help")) {
        notification = new NotificationIcon;
        addChild(notification);
    }

   private:
    // For drawing red dot in the Help Button for when there is a notification
    NotificationIcon* notification;

    void onAction(const ActionEvent& e) override {
        ui::Menu* menu = createMenu();
        menu->cornerFlags = BND_CORNER_TOP;
        menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));

        menu->addChild(new ui::MenuSeparator);

        menu->addChild(createSubmenuItem(
            "🌐 " + string::translate("MenuBar.help.language"), "",
            [=](ui::Menu* menu) { appendLanguageMenu(menu); }));

        menu->addChild(
            createMenuItem(string::translate("MenuBar.help.tips"), "",
                           [=]() { getScene()->addChild(tipWindowCreate()); }));

        menu->addChild(createMenuItem(
            string::translate("MenuBar.help.manual"),
            widget::getKeyCommandName(GLFW_KEY_F1, 0),
            [=]() { system::openBrowser("https://vcvrack.com/manual"); }));

        if (!settings::isNotVCVRack) {
            menu->addChild(createMenuItem(
                string::translate("MenuBar.help.support"), "",
                [=]() { system::openBrowser("https://vcvrack.com/support"); }));

            menu->addChild(createMenuItem("VCVRack.com", "", [=]() {
                system::openBrowser("https://vcvrack.com/");
            }));
        }

        menu->addChild(new ui::MenuSeparator);

        menu->addChild(
            createMenuItem(string::translate("MenuBar.help.userFolder"), "",
                           [=]() { system::openDirectory(asset::user("")); }));

        if (settings::isNotVCVRack) {
            // Show VCV Rack changelog
            menu->addChild(createMenuItem(
                string::translate("MenuBar.help.changelog"), "", [=]() {
                    system::openBrowser(
                        "https://github.com/VCVRack/Rack/blob/v2/CHANGELOG.md");
                }));
        } else {
            // Show Non VCV Rack changelog
            menu->addChild(createMenuItem(
                string::translate("MenuBar.help.changelog"), "", [=]() {
                    system::openBrowser(
                        "https://github.com/skibu/LiminalRack/blob/v2/docs/"
                        "liminalChangelog.md");
                }));
        }

        // For VCV Rack make getting updates easy. But this doesn't work for
        // forks like Liminal
        if (!settings::isNotVCVRack) {
            if (library::isAppUpdateAvailable()) {
                // If there is a new version of app available then create menu
                // button to update to it
                menu->addChild(createMenuItem(
                    string::f(string::translate("MenuBar.help.update"),
                              APP_NAME),
                    APP_VERSION + " → " + library::getAppVersion(),
                    [=]() { system::openBrowser(library::getAppDownloadUrl()); }));
            } else if (!settings::autoCheckUpdates && !settings::devMode) {
                // Create button for checking for update
                menu->addChild(createMenuItem(
                    string::f(string::translate("MenuBar.help.checkUpdate"),
                              APP_NAME),
                    "",
                    [=]() {
                        std::thread t([&]() { library::checkAppUpdate(); });
                        t.detach();
                    },
                    false, true));
            }
        }
	}

	void step() override {
		// For VCV Rack make getting updates easy. But this doesn't work for forks like Liminal
		if (!settings::isNotVCVRack) {
			// Light up red notification dot on Help button if an update is available
			notification->setPos(math::Vec(0, 0));
			notification->setVisible(library::isAppUpdateAvailable());
		} else {
			// Not VCV rack so always hide notification since can't update app in usual way
			notification->setVisible(false);
		}
		MenuButton::step();
	}
};

////////////////////
// InfoBar - displays frame rate, cpu, and app name/version info
////////////////////

/** Returns the cached last frame rate. By using a cached value
 * that only updates every SECS_BETWEEN_UPDATES seconds the display
 * of the value in the UI doesn't flicker as much. This is useful
 * since can have a frame rate of 30fps, where user cannot read
 * a value that updates every frame.
 */
static double getCachedLastFrameRate() {
    static double cachedLastFrameRate = 0.0;
    static double lastUpdateTime = 0.0;
    static double SECS_BETWEEN_UPDATES = 0.1;

    // Update every SECS_BETWEEN_UPDATES
    double currentTime = system::getTime();
    if (currentTime - lastUpdateTime >= SECS_BETWEEN_UPDATES) {
        cachedLastFrameRate = getWindow()->getLastFrameRate();
        lastUpdateTime = currentTime;
    }
    return cachedLastFrameRate;
}

/** Returns the cached potential frame rate. By using a cached value
 * that only updates every SECS_BETWEEN_UPDATES seconds the display
 * of the value in the UI doesn't flicker as much. This is useful
 * since can have a frame rate of 30fps, where user cannot read
 * a value that updates every frame.
 */
static double getCachedPotentialFrameRate() {
    static double cachedPotentialFrameRate = 0.0;
    static double lastUpdateTime = 0.0;
    static double SECS_BETWEEN_UPDATES = 0.2;

    // Update every SECS_BETWEEN_UPDATES
    double currentTime = system::getTime();
    if (currentTime - lastUpdateTime >= SECS_BETWEEN_UPDATES) {
        cachedPotentialFrameRate = getWindow()->getPotentialFrameRate();
        lastUpdateTime = currentTime;
    }
    return cachedPotentialFrameRate;
}

/** For displaying system values like cpu and fps */
class InfoLabel : public ui::Label {
    void step() override {
        std::string label = "";

        // If window wide enough display frame rate and CPU meter
        if (getWidth() >= 460) {
            double lastFps = getCachedLastFrameRate();
            // No point in showing more than 120fps
            double potentialFps = std::min(120.0, getCachedPotentialFrameRate());
            double meterAveragePct = getEngine()->getMeterAverage();
            // meterMaxPct not used currently since not useful to user
            // double meterMaxPct = getEngine()->getMeterMax();  
            label +=
                string::f(string::translate("MenuBar.infoLabel"), lastFps,
                          potentialFps, meterAveragePct);
            label += "   ";
        }

        // Add in app and OS name, but remove double spaces from appAndOsName
        std::string appAndOsName = APP_NAME + " " + APP_EDITION_NAME + " " + APP_VERSION +
                       " " + APP_OS_NAME + " " + APP_CPU_NAME;
        string::replaceAll(appAndOsName, "  ", " ");
        label += appAndOsName;

        // Use the completed label
        setText(label);

        // Figure out dimensions and draw label
        Label::step();
    }
};

////////////////////
// MenuBar
////////////////////

/**
 * The main menu bar for the application. Contains a buncch of buttons,
 * one for each pull down menu. Example buttons are FileButton and LibraryButton.
 * These button classes inheret from MenuButton, which is used to draw the buttons.
 */
struct MenuBar : widget::OpaqueWidget {
	/* For drawing in menu bar some greyed out info, like CPU and Rack version */
	InfoLabel* infoLabel;

	MenuBar() {
		const float margin = 3.0;
		setHeight(rack::settings::bndWidgetHeight + 2 * margin);

		ui::SequentialLayout* layout = new ui::SequentialLayout;
		layout->setMargin(math::Vec(margin, margin));
		// Set some space between the menu items so that they are easy to differentiate
		layout->setMinSpacing(math::Vec(15.0, 0));
		addChild(layout);

		FileButton* fileButton = new FileButton();
		layout->addChild(fileButton);

		EditButton* editButton = new EditButton();
		layout->addChild(editButton);

		ViewButton* viewButton = new ViewButton();
		layout->addChild(viewButton);

		EngineButton* engineButton = new EngineButton();
		layout->addChild(engineButton);

		LibraryButton* libraryButton = new LibraryButton();
		layout->addChild(libraryButton);

		HelpButton* helpButton = new HelpButton();
		layout->addChild(helpButton);

        // To display CPU and other such info
		infoLabel = new InfoLabel();
		infoLabel->setWidth(600);
		infoLabel->setAlignment(ui::Label::RIGHT_ALIGNMENT);
        infoLabel->setFontSize(16);
        infoLabel->setFontFaceOverride(settings::systemMonospacedFontFileName);
        // Lower a bit so alignts vertically with menu buttons
        infoLabel->setYOffset(7);
		layout->addChild(infoLabel);
	}

	void draw(const DrawArgs& args) override {
		bndMenuBackground(args.vg, 0.0, 0.0, getWidth(), getHeight(), BND_CORNER_ALL);
		bndBevel(args.vg, 0.0, 0.0, getWidth(), getHeight());

		Widget::draw(args);
	}

	void step() override {
		Widget::step();
		infoLabel->setWidth(getWidth() - infoLabel->getX() - 5);

        // Setting 50% alpha prevents Label from using the default UI
        // theme color, so set the color manually here.
        infoLabel->setColor(
            color::alpha(bndGetTheme()->regularTheme.textColor, 0.5));
}
};

}  // namespace menuBar

widget::Widget* createMenuBar() {
    menuBar::MenuBar* menuBar = new menuBar::MenuBar;
    return menuBar;
}

void appendLanguageMenu(ui::Menu* menu) {
	for (const std::string& language : string::getLanguages()) {
		menu->addChild(createCheckMenuItem(string::translate("language", language), "", [=]() {
			return settings::language == language;
		}, [=]() {
			if (settings::language == language)
				return;
			settings::language = language;
			// Request restart
			std::string msg = string::f(string::translate("MenuBar.help.language.restart"), string::translate("language"));
			if (osdialog_message(OSDIALOG_INFO, OSDIALOG_OK_CANCEL, msg.c_str())) {
				getWindow()->close();
				settings::restart = true;
			}
		}));
	}
}


} // namespace app
} // namespace rack
