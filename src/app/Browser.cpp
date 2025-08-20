/**
 * The Browser is the window that shows the modules the user has
 * available and can add to their rack.
 */

#include<set>
#include <algorithm>
#include <thread>

#include <app/Browser.hpp>
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <widget/ZoomWidget.hpp>
#include <ui/MenuOverlay.hpp>
#include <ui/ScrollWidget.hpp>
#include <ui/SequentialLayout.hpp>
#include <ui/Label.hpp>
#include <ui/Slider.hpp>
#include <ui/TextField.hpp>
#include <ui/MenuItem.hpp>
#include <ui/MenuSeparator.hpp>
#include <ui/Button.hpp>
#include <ui/ChoiceButton.hpp>
#include <ui/RadioButton.hpp>
#include <ui/OptionButton.hpp>
#include <ui/Tooltip.hpp>
#include <app/ModuleWidget.hpp>
#include <app/Scene.hpp>
#include <plugin.hpp>
#include <context.hpp>
#include <engine/Engine.hpp>
#include <plugin/Model.hpp>
#include <string.hpp>
#include <history.hpp>
#include <settings.hpp>
#include <system.hpp>
#include <tag.hpp>
#include <helpers.hpp>
#include <FuzzySearchDatabase.hpp>
#include <componentlibrary.hpp>


namespace rack {
namespace app {
namespace browser {


static fuzzysearch::Database<plugin::Model*> modelDb;


static void modelDbInit() {
	modelDb = fuzzysearch::Database<plugin::Model*>();
	modelDb.setWeights({0.9f, 0.75f, 1.0f, 0.8f, 0.9f});
	modelDb.setThreshold(0.5f);

	// Iterate plugins
	for (plugin::Plugin* plugin : plugin::plugins) {
		// Iterate model in plugin
		for (plugin::Model* model : plugin->models) {
			// Get search fields for model
			std::string tagStr;
			for (int tagId : model->tagIds) {
				// If non-English, add translation of tag before English tags
				if (settings::language != "en") {
					tagStr += string::translate("tag." + tag::getTag(tagId), settings::language);
					tagStr += " ";
				}
				// Add all aliases of a tag
				for (const std::string& tagAlias : tag::tagAliases[tagId]) {
					tagStr += tagAlias;
					tagStr += " ";
				}
			}
			std::vector<std::string> fields = {
				model->plugin->brand,
				model->plugin->name,
				model->name,
				model->description,
				tagStr,
			};
			// DEBUG("%s; %s; %s; %s; %s", fields[0].c_str(), fields[1].c_str(), fields[2].c_str(), fields[3].c_str(), fields[4].c_str());
			modelDb.addEntry(model, fields);
		}
	}
}


/** Called when user clicks on module to choose it */
static ModuleWidget* chooseModel(plugin::Model* model) {
	// Record usage
	settings::ModuleInfo& mi = settings::moduleInfos[model->plugin->slug][model->slug];
	mi.added++;
	mi.lastAdded = system::getUnixTime();

	history::ComplexAction* h = new history::ComplexAction;
	h->name = string::translate("Browser.history.addModule");

	// Create Module and ModuleWidget
	INFO("Creating module %s", model->getFullName().c_str());
	engine::Module* module = model->createModule();
	APP->engine->addModule(module);

	INFO("Creating module widget %s", model->getFullName().c_str());
	ModuleWidget* moduleWidget = model->createModuleWidget(module);

	APP->scene->rack->deselectAll();
	APP->scene->rack->updateModuleOldPositions();
	APP->scene->rack->addModuleAtMouse(moduleWidget);
	h->push(APP->scene->rack->getModuleDragAction());

	// Load template preset
	moduleWidget->loadTemplate();

	// history::ModuleAdd
	history::ModuleAdd* ha = new history::ModuleAdd;
	// This serializes the module so redoing returns to the current state.
	ha->setModule(moduleWidget);
	h->push(ha);

	APP->history->push(h);

	// Hide Module Browser
	APP->scene->browser->hide();

	return moduleWidget;
}


// Widgets


struct Browser;


/** 
 * For covering up everything outside the Browser Window and making
 * that area darker. This way the user focus is on the Browser Window
 * yet they still see that the rack window is there, right underneath.
 */
struct BrowserOverlay : ui::MenuOverlay {
	void step() override {
		// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
		if (isVisible())
			MenuOverlay::step();
	}

	void onAction(const ActionEvent& e) override {
		// Hide instead of requestDelete()
		hide();
	}
};


struct ModuleWidgetContainer : widget::Widget {
	void draw(const DrawArgs& args) override {
		Widget::draw(args);
		Widget::drawLayer(args, 1);
	}
};


struct ModelBox : widget::OpaqueWidget {
	plugin::Model* model;
	ui::Tooltip* tooltip = NULL;
	// Lazily created widgets
	widget::Widget* previewWidget = NULL;
	widget::ZoomWidget* zoomWidget = NULL;
	widget::FramebufferWidget* fb = NULL;
	ModuleWidgetContainer* mwc = NULL;
	ModuleWidget* moduleWidget = NULL;

	ModelBox() {
		updateZoom();
	}

	void setModel(plugin::Model* model) {
		this->model = model;
	}

	void updateZoom() {
		float zoom = std::pow(2.f, settings::browserZoom);

		if (previewWidget) {
			fb->setDirty();
			zoomWidget->setZoom(zoom);
			box.size.x = moduleWidget->box.size.x * zoom;
		}
		else {
			// Approximate size as 12HP before we know the actual size.
			// We need a nonzero size, otherwise too many ModelBoxes will lazily render in the same frame.
			box.size.x = 12 * RACK_GRID_WIDTH * zoom;
		}
		box.size.y = RACK_GRID_HEIGHT * zoom;
		box.size = box.size.ceil();
	}

	void createPreview() {
		if (previewWidget)
			return;

		previewWidget = new widget::TransparentWidget;
		addChild(previewWidget);

		zoomWidget = new widget::ZoomWidget;
		previewWidget->addChild(zoomWidget);

		fb = new widget::FramebufferWidget;
		if (APP->window->pixelRatio < 2.0) {
			// Small details draw poorly at low DPI, so oversample when drawing to the framebuffer
			fb->oversample = 2.0;
		}
		zoomWidget->addChild(fb);

		mwc = new ModuleWidgetContainer;
		fb->addChild(mwc);

		INFO("Creating module widget %s", model->getFullName().c_str());
		moduleWidget = model->createModuleWidget(NULL);
		mwc->addChild(moduleWidget);
		mwc->box.size = moduleWidget->box.size;

		// Step ModuleWidget so it can set its default appearance.
		moduleWidget->step();

		updateZoom();
	}

	void draw(const DrawArgs& args) override {
		// Lazily create preview when drawn
		createPreview();

		// Draw shadow
		nvgBeginPath(args.vg);
		float r = 10; // Blur radius
		float c = 5; // Corner radius
		math::Rect shadowBox = box.zeroPos().grow(math::Vec(r, r));
		nvgRect(args.vg, RECT_ARGS(shadowBox));
		NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.5);
		nvgFillPaint(args.vg, nvgBoxGradient(args.vg, 0, 0, box.size.x, box.size.y, c, r, shadowColor, color::BLACK_TRANSPARENT));
		nvgFill(args.vg);

		// To avoid blinding the user when rack brightness is low, draw framebuffer with the same brightness.
		float b = math::clamp(settings::rackBrightness + 0.2f, 0.f, 1.f);
		nvgGlobalTint(args.vg, nvgRGBAf(b, b, b, 1));

		OpaqueWidget::draw(args);

		// Draw favorite border if module has been favorited
		const settings::ModuleInfo* mi = settings::getModuleInfo(model->plugin->slug, model->slug);
		if (mi && mi->favorite) {
			nvgBeginPath(args.vg);
			math::Rect borderBox = box.zeroPos();
			nvgRect(args.vg, RECT_ARGS(borderBox));
			nvgStrokeWidth(args.vg, 2);
			nvgStrokeColor(args.vg, componentlibrary::SCHEME_YELLOW);
			nvgStroke(args.vg);
		}
	}

	void step() override {
		OpaqueWidget::step();
	}

	void setTooltip(ui::Tooltip* tooltip) {
		if (this->tooltip) {
			this->tooltip->requestDelete();
			this->tooltip = NULL;
		}

		if (tooltip) {
			APP->scene->addChild(tooltip);
			this->tooltip = tooltip;
		}
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			ModuleWidget* mw = chooseModel(model);

			// Pretend the moduleWidget was clicked so it can be dragged in the RackWidget
			e.consume(mw);

			// Set the drag position at the center of the module
			mw->dragOffset() = mw->box.size.div(2);
			// Disable dragging temporarily until the mouse has moved a bit.
			mw->dragEnabled() = false;
		}

		// Toggle favorite
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			model->setFavorite(!model->isFavorite());
			e.consume(this);
		}

		// Open context menu on right-click
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
			createContextMenu();
			e.consume(this);
		}
	}

	void onHoverKey(const HoverKeyEvent& e) override {
		if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
			if (e.isKeyCommand(GLFW_KEY_F1, RACK_MOD_CTRL)) {
				system::openBrowser(model->getManualUrl());
				e.consume(this);
			}
		}

		if (e.isConsumed())
			return;
		OpaqueWidget::onHoverKey(e);
	}

	ui::Tooltip* createTooltip() {
		std::string text;
		text += model->name;
		text += "\n";
		text += model->plugin->brand;
		// Description
		if (model->description != "") {
			text += "\n" + model->description;
		}
		// Tags
		text += "\n\n";
		text += string::translate("Browser.tooltipTags");
		std::vector<std::string> tags;
		for (int tagId : model->tagIds) {
			std::string tag = string::translate("tag." + tag::getTag(tagId));
			tags.push_back(tag);
		}
		text += string::join(tags, ", ");
		ui::Tooltip* tooltip = new ui::Tooltip;
		tooltip->text = text;
		return tooltip;
	}

	void onEnter(const EnterEvent& e) override {
		setTooltip(createTooltip());
	}

	void onLeave(const LeaveEvent& e) override {
		setTooltip(NULL);
	}

	void onHide(const HideEvent& e) override {
		// Hide tooltip
		setTooltip(NULL);
		OpaqueWidget::onHide(e);
	}

	void createContextMenu() {
		ui::Menu* menu = createMenu();

		menu->addChild(createMenuLabel(model->name));
		menu->addChild(createMenuLabel(model->plugin->brand));
		model->appendContextMenu(menu, true);
	}
};


struct BrowserSearchField : ui::TextField {
	Browser* browser;

	void step() override {
		// Steal focus when step is called
		APP->event->setSelectedWidget(this);
		TextField::step();
	}

	void onSelectKey(const SelectKeyEvent& e) override;
	void onChange(const ChangeEvent& e) override;
	void onAction(const ActionEvent& e) override;

	void onHide(const HideEvent& e) override {
		APP->event->setSelectedWidget(NULL);
		ui::TextField::onHide(e);
	}

	void onShow(const ShowEvent& e) override {
		selectAll();
		TextField::onShow(e);
	}
};


struct FavoriteQuantity : Quantity {
	Browser* browser;
	void setValue(float value) override;
	float getValue() override;
};


struct ClearButton : ui::Button {
	Browser* browser;
	void onAction(const ActionEvent& e) override;
};


struct BrandItem : ui::MenuItem {
	Browser* browser;
	std::string brand;
	void onAction(const ActionEvent& e) override;
	void step() override;
};


struct BrandButton : ui::ChoiceButton {
	Browser* browser;

	void onAction(const ActionEvent& e) override;
	void step() override;
};


/** An item for the tag menu. Causes the TagItem::onAction() method to be called on a click. */
struct TagItem : ui::MenuItem {
	Browser* browser;
	int tagId;
	void onAction(const ActionEvent& e) override;
	void step() override;
};


struct TagButton : ui::ChoiceButton {
	Browser* browser;

	void onAction(const ActionEvent& e) override;
	void step() override;
};


static std::vector<std::string> getSortNames() {
	return {
		string::translate("Browser.sort.lastUpdated"),
		string::translate("Browser.sort.lastUsed"),
		string::translate("Browser.sort.mostUsed"),
		string::translate("Browser.sort.brand"),
		string::translate("Browser.sort.moduleName"),
		string::translate("Browser.sort.random"),
	};
};


struct SortButton : ui::ChoiceButton {
	Browser* browser;

	void onAction(const ActionEvent& e) override;

	void step() override {
		text = string::translate("Browser.sort");
		text += getSortNames()[settings::browserSort];
		text = string::ellipsize(text, 20);
		ChoiceButton::step();
	}
};


struct ZoomButton : ui::ChoiceButton {
	Browser* browser;

	void onAction(const ActionEvent& e) override;

	void step() override {
		text = string::translate("Browser.zoom");
		text += string::f("%.0f%%", std::pow(2.f, settings::browserZoom) * 100.f);
		ChoiceButton::step();
	}
};


struct UrlButton : ui::Button {
	std::string url;
	void onAction(const ActionEvent& e) override {
		system::openBrowser(url);
	}
};

/**
 * BrowserHeader is the widget that is at the head of the Module Browser. It's only purpose
 * is to override SequentialLayout onDraw() so that a specified font can be used for all
 * of the children of the header. It uses the same layout algorithm as SequentialLayout.
 */
class BrowserHeader : public ui::SequentialLayout {
   public:
    BrowserHeader() {
        alignment = CENTER_ALIGNMENT;
    }

    static const int HeaderFontSize = 16;
    static const int HeaderWidgetHeight = 24;

    /** Override draw() so that all children of the header are drawn with a specified font size.
     * This is necessary since the header contains buttons and labels that need to be drawn with a
     * different font size than the rest of the browser. 
     */
    void draw(const DrawArgs& args) override {
        // Temporarily store the current font size
        int originalFontSize = settings::getLabelFontSize();
        int originalWidgetHeight = settings::getWidgetHeight();

        // Set the font size for the header
        settings::setLabelFontSize(HeaderFontSize);
		settings::setWidgetHeight(BrowserHeader::HeaderWidgetHeight);

        // Draw all the children of the header using that font size
        SequentialLayout::draw(args);

        // Restore the font size
        settings::setLabelFontSize(originalFontSize);
        settings::setWidgetHeight(originalWidgetHeight);
    }
};

/** The actual Browser class. The Browser is the window that shows the modules 
 * the user has available and can add to their rack.
 */
struct Browser : widget::OpaqueWidget {
    ui::Label* titleLabel;
    ui::SequentialLayout* headerLayout;
    BrowserSearchField* searchField;
    BrandButton* brandButton;
    TagButton* tagButton;
    FavoriteQuantity* favoriteQuantity;
    ui::OptionButton* favoriteButton;
    ClearButton* clearButton;
    ui::Label* countLabel;

    ui::ScrollWidget* moduleScroll;
    widget::Widget* moduleMargin;
    ui::SequentialLayout* moduleLayoutContainer;

    std::string search;
    std::string brand;
    std::set<int> tagIds = {};
    bool favorite = false;
    bool lastPreferDarkPanels = false;

    // Caches and temporary state
    std::map<plugin::Model*, float> prefilteredModelScores;
    std::map<plugin::Model*, int> modelOrders;

    Browser() {
        // Margin used for the small border around the Browser window
        const float outerMargin = 8.0;

        // Browser top label
        titleLabel = new ui::Label;
        titleLabel->text = string::translate("Browser.title");
        titleLabel->fontSize = 40;
        titleLabel->alignment = ui::Label::Alignment::CENTER_ALIGNMENT;
        titleLabel->box.pos.x = outerMargin;
        titleLabel->box.pos.y = outerMargin + 2.0;
        titleLabel->box.size.y = titleLabel->fontSize * 0.7 + 2.0; // Don't need full height for label
        addChild(titleLabel);

        // Header
        headerLayout = new BrowserHeader();
        headerLayout->box.pos = math::Vec(0, outerMargin + titleLabel->box.size.y);
        headerLayout->box.size.y = 0; // Height will be set later
        headerLayout->margin = math::Vec(outerMargin, outerMargin);
        headerLayout->spacing = math::Vec(outerMargin, outerMargin);
        addChild(headerLayout);

        // Need to set desired widgetHeight here instead of when drawing since
        // box.size.y is set in Button's constructor
        int originalWidgetHeight = settings::getWidgetHeight();
		settings::setWidgetHeight(BrowserHeader::HeaderWidgetHeight);

        // Note: for the header widgets size.x using original values for VCV Rack,
        // but then adjusting them to the font size actually being used here.
        // This way it is easy to change the font size simply by setting HeaderFontSize.

        searchField = new BrowserSearchField;
        searchField->box.size.x = 150 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        searchField->placeholder = string::translate("Browser.searchModules");
        searchField->browser = this;
        headerLayout->addChild(searchField);

        brandButton = new BrandButton;
        brandButton->box.size.x = 150 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        brandButton->browser = this;
        headerLayout->addChild(brandButton);

        tagButton = new TagButton;
        tagButton->box.size.x = 150 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        tagButton->browser = this;
        headerLayout->addChild(tagButton);

        favoriteQuantity = new FavoriteQuantity;
        favoriteQuantity->browser = this;

        favoriteButton = new ui::OptionButton;
        favoriteButton->quantity = favoriteQuantity;
        favoriteButton->text = string::translate("Browser.favorites");
        favoriteButton->box.size.x = 70 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        headerLayout->addChild(favoriteButton);

        clearButton = new ClearButton;
        clearButton->box.size.x = 90 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        clearButton->text = string::translate("Browser.resetFilters");
        clearButton->browser = this;
        headerLayout->addChild(clearButton);

        // The count of the number of modules being displayed is not that important since
        // can see the modules. And it takes up precious space. Plus there are already 
        // lots of other action widgets, complicating the UI. Therefore don't display it
        // unless VCV Rack where want UI to be consistent.
        if (!settings::isNotVCVRack) {
            countLabel = new ui::Label;
            countLabel->box.size.x = 110 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
            headerLayout->addChild(countLabel);
        }

        SortButton* sortButton = new SortButton;
        sortButton->box.size.x = 130 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        sortButton->browser = this;
        headerLayout->addChild(sortButton);

        ZoomButton* zoomButton = new ZoomButton;
        zoomButton->box.size.x = 100 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        zoomButton->browser = this;
        headerLayout->addChild(zoomButton);

        UrlButton* libraryButton = new UrlButton;
        libraryButton->box.size.x = 170 * BrowserHeader::HeaderFontSize / BND_LABEL_FONT_SIZE;
        libraryButton->text = string::translate("Browser.browseLibrary");
        libraryButton->url = "https://library.vcvrack.com/";
        headerLayout->addChild(libraryButton);

        // Restore original height
        settings::setWidgetHeight(originalWidgetHeight);

        // Create scrollable module container
        moduleScroll = new ui::ScrollWidget;
        moduleScroll->box.pos.y = rack::settings::bndWidgetHeight;
        addChild(moduleScroll);

        moduleMargin = new widget::Widget;
        moduleScroll->container->addChild(moduleMargin);

        moduleLayoutContainer = new ui::SequentialLayout;
        moduleLayoutContainer->margin = math::Vec(outerMargin, 2);  // Add 2 pixels for favorites border
        moduleLayoutContainer->spacing = math::Vec(outerMargin, outerMargin);
        moduleMargin->addChild(moduleLayoutContainer);

        resetModuleBoxes();
        clearSelectorsInHeader();
    }

	~Browser() {
		delete favoriteQuantity;
	}

	void resetModuleBoxes() {
		moduleLayoutContainer->clearChildren();
		modelOrders.clear();
		// Iterate plugins
		for (plugin::Plugin* plugin : plugin::plugins) {
			// Iterate models in plugin
			int modelIndex = 0;
			for (plugin::Model* model : plugin->models) {
				// Create ModelBox
				ModelBox* modelBox = new ModelBox;
				modelBox->setModel(model);
				moduleLayoutContainer->addChild(modelBox);

				modelOrders[model] = modelIndex;
				modelIndex++;
			}
		}
	}

	void updateZoom() {
		moduleScroll->offset = math::Vec();

		for (Widget* w : moduleLayoutContainer->children) {
			ModelBox* mb = reinterpret_cast<ModelBox*>(w);
			assert(mb);
			mb->updateZoom();
		}
	}

	void step() override {
		// Determine size of the Browser window. Make it 30 units smaller than
		// the main window so that can see that the window is on top of the rack display
		box = parent->box.zeroPos().grow(math::Vec(-30, -30));

		// Determine horizontal layout of titleLabel
		titleLabel->box.size.x = box.size.x; 

		// The modules to edges of window margin
		const float rightAndBottomMargin = 10; // FIXME should verify what this really does

		// Now that know how big enclosing window is can set position and sizes of the 
		// containers. First, set width of the headerLayout widget.
		headerLayout->box.size.x = box.size.x;

		// Set the position and size of the scrollable container that contains all the modules.
		// Make it so there is a margin at the bottom.
		moduleScroll->box.pos = headerLayout->box.getBottomLeft();
		moduleScroll->box.size = box.size.minus(moduleScroll->box.pos) - math::Vec(0, 10);

		// Set the size of the moduleMargin which is inside of the moduleScroll
		moduleMargin->box.size.x = moduleScroll->box.size.x;
		moduleMargin->box.size.y = moduleLayoutContainer->box.size.y + rightAndBottomMargin;
		moduleLayoutContainer->box.size.x = moduleMargin->box.size.x - rightAndBottomMargin;

		// Check if preferDarkPanels has changed
		if (settings::preferDarkPanels != lastPreferDarkPanels) {
			lastPreferDarkPanels = settings::preferDarkPanels;
			// Request module framebuffers to re-render
			Widget::DirtyEvent eDirty;
			moduleLayoutContainer->onDirty(eDirty);
		}

		OpaqueWidget::step();
	}

	void draw(const DrawArgs& args) override {
		// Draw a light gray background
		NVGcolor bg_color = settings::moduleBrowserBg;
		NVGcolor outline_color = color::brightness(bg_color) < 0.5f
										? color::lerp(bg_color, color::WHITE,
													0.1)  // Light outline for dark background
										: color::lerp(bg_color, color::BLACK,
													0.1);  // Dark outline for light background
		float radius = 10.0; // Use a noticable radius to further differentiate browser window
        bndBackgroundColor(args.vg, 0.0, 0.0, box.size.x, box.size.y, radius, bg_color, 
			outline_color);

		Widget::draw(args);
	}

    bool isModelVisible(plugin::Model* model, const std::string& brand, std::set<int> tagIds, bool favorite) {
		// Filter hidden
		if (model->hidden)
			return false;

		// Filter moduleInfo setting if enabled is false
		settings::ModuleInfo* mi = settings::getModuleInfo(model->plugin->slug, model->slug);
		if (mi && !mi->enabled)
			return false;

		// Filter if not whitelisted by library
		if (!settings::isModuleWhitelisted(model->plugin->slug, model->slug))
			return false;

		// Filter favorites
		if (favorite) {
			if (!mi || !mi->favorite)
				return false;
		}

		// Filter brand
		if (!brand.empty()) {
			if (model->plugin->brand != brand)
				return false;
		}

		// Filter tag
		for (int tagId : tagIds) {
			auto it = std::find(model->tagIds.begin(), model->tagIds.end(), tagId);
			if (it == model->tagIds.end())
				return false;
		}

		return true;
	};

	// Determines if there is at least 1 visible Model with a given brand and tag
	bool hasVisibleModel(const std::string& brand, std::set<int> tagIds, bool favorite) {
		for (const auto& pair : prefilteredModelScores) {
			plugin::Model* model = pair.first;
			if (isModelVisible(model, brand, tagIds, favorite))
				return true;
		}
		return false;
	};

	template <typename F>
	void sortModels(F f) {
		moduleLayoutContainer->children.sort([&](Widget* w1, Widget* w2) {
			ModelBox* m1 = reinterpret_cast<ModelBox*>(w1);
			ModelBox* m2 = reinterpret_cast<ModelBox*>(w2);
			return f(m1) < f(m2);
		});
	}

	void refresh() {
		// Reset scroll position
		moduleScroll->offset = math::Vec();

		prefilteredModelScores.clear();

		// Filter ModelBoxes by brand and tag
		for (Widget* w : moduleLayoutContainer->children) {
			ModelBox* m = reinterpret_cast<ModelBox*>(w);
			m->setVisible(isModelVisible(m->model, brand, tagIds, favorite));
		}

		// Filter and sort by search results
		if (search.empty()) {
			// Add all models to prefilteredModelScores with scores of 1
			for (Widget* w : moduleLayoutContainer->children) {
				ModelBox* m = reinterpret_cast<ModelBox*>(w);
				prefilteredModelScores[m->model] = 1.f;
			}

			// Sort ModelBoxes
			if (settings::browserSort == settings::BROWSER_SORT_UPDATED) {
				sortModels([this](ModelBox* m) {
					plugin::Plugin* p = m->model->plugin;
					int modelOrder = get(modelOrders, m->model, 0);
					return std::make_tuple(-p->modifiedTimestamp, p->brand, p->name, modelOrder);
				});
			}
			else if (settings::browserSort == settings::BROWSER_SORT_LAST_USED) {
				sortModels([this](ModelBox* m) {
					plugin::Plugin* p = m->model->plugin;
					const settings::ModuleInfo* mi = settings::getModuleInfo(p->slug, m->model->slug);
					double lastAdded = mi ? mi->lastAdded : -INFINITY;
					int modelOrder = get(modelOrders, m->model, 0);
					return std::make_tuple(-lastAdded, -p->modifiedTimestamp, p->brand, p->name, modelOrder);
				});
			}
			else if (settings::browserSort == settings::BROWSER_SORT_MOST_USED) {
				sortModels([this](ModelBox* m) {
					plugin::Plugin* p = m->model->plugin;
					const settings::ModuleInfo* mi = settings::getModuleInfo(p->slug, m->model->slug);
					int added = mi ? mi->added : 0;
					double lastAdded = mi ? mi->lastAdded : -INFINITY;
					int modelOrder = get(modelOrders, m->model, 0);
					return std::make_tuple(-added, -lastAdded, -p->modifiedTimestamp, p->brand, p->name, modelOrder);
				});
			}
			else if (settings::browserSort == settings::BROWSER_SORT_BRAND) {
				sortModels([this](ModelBox* m) {
					plugin::Plugin* p = m->model->plugin;
					int modelOrder = get(modelOrders, m->model, 0);
					return std::make_tuple(p->brand, p->name, modelOrder);
				});
			}
			else if (settings::browserSort == settings::BROWSER_SORT_NAME) {
				sortModels([](ModelBox* m) {
					plugin::Plugin* p = m->model->plugin;
					return std::make_tuple(m->model->name, p->brand);
				});
			}
			else if (settings::browserSort == settings::BROWSER_SORT_RANDOM) {
				std::map<ModelBox*, uint64_t> randomOrder;
				for (Widget* w : moduleLayoutContainer->children) {
					ModelBox* m = reinterpret_cast<ModelBox*>(w);
					randomOrder[m] = random::u64();
				}
				sortModels([&](ModelBox* m) {
					return get(randomOrder, m, 0);
				});
			}
		}
		else {
			// Score results against search query
			auto results = modelDb.search(search);
			// DEBUG("=============");
			for (auto& result : results) {
				prefilteredModelScores[result.key] = result.score;
				// DEBUG("%s %s\t\t%f", result.key->plugin->slug.c_str(), result.key->slug.c_str(), result.score);
			}
			// Sort by score
			sortModels([&](ModelBox* m) {
				return -get(prefilteredModelScores, m->model, 0.f);
			});
			// Filter by whether the score is above the threshold
			for (Widget* w : moduleLayoutContainer->children) {
				ModelBox* m = reinterpret_cast<ModelBox*>(w);
				assert(m);
				if (m->isVisible()) {
					if (prefilteredModelScores.find(m->model) == prefilteredModelScores.end())
						m->hide();
				}
			}
		}

        // Only update countLabel if it was actually created
        if (countLabel) {
            // Count visible modules
            int count = 0;
            for (Widget* w : moduleLayoutContainer->children) {
                if (w->isVisible()) count++;
            }
            countLabel->text =
                (count == 1) ? string::translate("Browser.modulesOne")
                                : string::f(string::translate("Browser.modulesMany"), count);
        }
        }

	void clearSelectorsInHeader() {
		search = "";
		searchField->setText("");
		brand = "";
		tagIds = {};
		favorite = false;
		refresh();
	}

	void onButton(const ButtonEvent& e) override {
		Widget::onButton(e);
		e.stopPropagating();
		// Consume all mouse buttons
		if (!e.isConsumed())
			e.consume(this);
	}

	void onHoverKey(const HoverKeyEvent& e) override {
		if (e.action == GLFW_PRESS) {
			// Secret key command to dump all visible modules into rack
			if (e.isKeyCommand(GLFW_KEY_F2, RACK_MOD_CTRL | GLFW_MOD_SHIFT | GLFW_MOD_ALT)) {
				int count = 0;
				for (widget::Widget* w : moduleLayoutContainer->children) {
					ModelBox* mb = dynamic_cast<ModelBox*>(w);
					if (!mb)
						continue;
					if (!mb->visible)
						continue;
					count++;
					DEBUG("Dumping into rack (%d): %s/%s", count, mb->model->plugin->slug.c_str(), mb->model->slug.c_str());
					chooseModel(mb->model);
				}
				e.consume(this);
			}
		}

		if (e.isConsumed())
			return;
		OpaqueWidget::onHoverKey(e);
	}
};


// Implementations to resolve dependencies


inline void FavoriteQuantity::setValue(float value) {
	browser->favorite = value;
	browser->refresh();
}

inline float FavoriteQuantity::getValue() {
	return browser->favorite;
}

inline void ClearButton::onAction(const ActionEvent& e) {
	browser->clearSelectorsInHeader();
}

inline void BrowserSearchField::onSelectKey(const SelectKeyEvent& e) {
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		// Backspace when the field is empty to clear filters.
		if (e.isKeyCommand(GLFW_KEY_BACKSPACE) || e.isKeyCommand(GLFW_KEY_BACKSPACE, RACK_MOD_CTRL)) {
			if (text == "") {
				browser->clearSelectorsInHeader();
				e.consume(this);
			}
		}
	}

	if (!e.getTarget())
		ui::TextField::onSelectKey(e);
}

inline void BrowserSearchField::onChange(const ChangeEvent& e) {
	browser->search = string::trim(text);
	browser->refresh();
}

inline void BrowserSearchField::onAction(const ActionEvent& e) {
	// Get first ModelBox
	ModelBox* mb = NULL;
	for (Widget* w : browser->moduleLayoutContainer->children) {
		if (w->isVisible()) {
			mb = reinterpret_cast<ModelBox*>(w);
			break;
		}
	}

	if (mb) {
		chooseModel(mb->model);
	}
}

inline void BrandItem::onAction(const ActionEvent& e) {
	if (browser->brand == brand)
		browser->brand = "";
	else
		browser->brand = brand;
	browser->refresh();
}

inline void BrandItem::step() {
	rightText = CHECKMARK(browser->brand == brand);
	MenuItem::step();
}

inline void BrandButton::onAction(const ActionEvent& e) {
    INFO("Clicked on Brand and got action "/*, e.context */);
	ui::Menu* menu = createMenu();
	menu->box.pos = getAbsoluteOffset(math::Vec(0, box.size.y));
	menu->box.size.x = box.size.x;

	BrandItem* noneItem = new BrandItem;
	noneItem->text = string::translate("Browser.allBrands");
	noneItem->brand = "";
	noneItem->browser = browser;
	menu->addChild(noneItem);

	menu->addChild(new ui::MenuSeparator);

	// Collect brands from all plugins
	std::set<std::string, string::CaseInsensitiveCompare> brands;
	for (plugin::Plugin* plugin : plugin::plugins) {
		brands.insert(plugin->brand);
	}

	for (const std::string& brand : brands) {
		BrandItem* brandItem = new BrandItem;
		brandItem->text = brand;
		brandItem->brand = brand;
		brandItem->browser = browser;
		brandItem->disabled = !browser->hasVisibleModel(brand, browser->tagIds, browser->favorite);
		menu->addChild(brandItem);
	}
}

inline void BrandButton::step() {
	text = string::translate("Browser.brand");
	if (!browser->brand.empty()) {
		text += ": ";
		text += browser->brand;
	}
	text = string::ellipsize(text, 20);
	ChoiceButton::step();
}

/** Called when user clicks on an item in the tag menu */
inline void TagItem::onAction(const ActionEvent& e) {
	auto it = browser->tagIds.find(tagId);
	bool isSelected = (it != browser->tagIds.end());

	if (tagId >= 0) {
		// Specific tag
		if (!e.isConsumed()) {
			// Multi select
			if (isSelected)
				browser->tagIds.erase(tagId);
			else
				browser->tagIds.insert(tagId);
			e.unconsume();
		}
		else {
			// Single select
			if (isSelected)
				browser->tagIds = {};
			else {
				browser->tagIds = {tagId};
			}
		}
	}
	else {
		// All tags
		browser->tagIds = {};
	}

	browser->refresh();
}

inline void TagItem::step() {
	// TODO Disable tags with no modules
	if (tagId >= 0) {
		auto it = browser->tagIds.find(tagId);
		bool isSelected = (it != browser->tagIds.end());
		rightText = CHECKMARK(isSelected);
	}
	else {
		rightText = CHECKMARK(browser->tagIds.empty());
	}
	MenuItem::step();
}

inline void TagButton::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	menu->box.pos = getAbsoluteOffset(math::Vec(0, box.size.y));
	menu->box.size.x = box.size.x;

	TagItem* noneItem = new TagItem;
	noneItem->text = string::translate("Browser.allTags");
	noneItem->tagId = -1;
	noneItem->browser = browser;
	menu->addChild(noneItem);

    if (settings::hasTouchscreen) {
        // Touchscreen, so Let user know they can select multiple tags by long click
        menu->addChild(createMenuLabel(string::translate("Browser.tagsSelectMultipleTouchScreen")));
    } else {
        // Not a touch screen so tell user they can use MOD_CTRL click to select multiple
        menu->addChild(createMenuLabel(widget::getKeyCommandName(0, RACK_MOD_CTRL) + string::translate("key.click") + string::translate("Browser.tagsSelectMultiple")));
    }
	menu->addChild(new ui::MenuSeparator);

	for (int tagId = 0; tagId < (int) tag::tagAliases.size(); tagId++) {
		TagItem* tagItem = new TagItem;
		std::string tag = string::translate("tag." + tag::getTag(tagId));
		tagItem->text = tag;
		tagItem->tagId = tagId;
		tagItem->browser = browser;
		tagItem->disabled = !browser->hasVisibleModel(browser->brand, {tagId}, browser->favorite);
		menu->addChild(tagItem);
	}
}

inline void TagButton::step() {
	text = string::translate("Browser.tags");
	if (!browser->tagIds.empty()) {
		text += ": ";
		bool firstTag = true;
		for (int tagId : browser->tagIds) {
			if (!firstTag)
				text += ", ";
			std::string tag = string::translate("tag." + tag::getTag(tagId));
			text += tag;
			firstTag = false;
		}
	}
	text = string::ellipsize(text, 20);
	ChoiceButton::step();
}

inline void SortButton::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	menu->box.pos = getAbsoluteOffset(math::Vec(0, box.size.y));
	menu->box.size.x = box.size.x;

	for (int sortId = 0; sortId <= settings::BROWSER_SORT_RANDOM; sortId++) {
		menu->addChild(createCheckMenuItem(getSortNames()[sortId], "",
			[=]() {return settings::browserSort == sortId;},
			[=]() {
				settings::browserSort = (settings::BrowserSort) sortId;
				browser->refresh();
			}
		));
	}
}

inline void ZoomButton::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
	menu->box.pos = getAbsoluteOffset(math::Vec(0, box.size.y));
	menu->box.size.x = box.size.x;

	for (float zoom = 1.f; zoom >= -2.f; zoom -= 0.5f) {
		menu->addChild(createCheckMenuItem(string::f("%.0f%%", std::pow(2.f, zoom) * 100.f), "",
			[=]() {return settings::browserZoom == zoom;},
			[=]() {
				if (zoom == settings::browserZoom)
					return;
				settings::browserZoom = zoom;
				browser->updateZoom();
			}
		));
	}
}


} // namespace browser



void browserInit() {
	browser::modelDbInit();
}


widget::Widget* browserCreate() {
    // Draw a dark area over the rest of the UI. This way user's focus is on
    // the Browser Window but they can still see that the Rack window is there, underneath.
	browser::BrowserOverlay* overlay = new browser::BrowserOverlay;
	overlay->bgColor = nvgRGBAf(0, 0, 0, 0.53); // Higher the value the darker things get

    // Now actually create the Browser window and add it to the overlay heirachy
	browser::Browser* browser = new browser::Browser;
	overlay->addChild(browser);

	return overlay;
}


} // namespace app
} // namespace rack
