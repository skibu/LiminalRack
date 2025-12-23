/**
 * The Browser is the window that shows the modules the user has
 * available and can add to their rack.
 */

#include<set>
#include <algorithm>
#include <thread>

#include <app/Browser.hpp>

#include <widget/TransparentWidget.hpp>
#include <widget/ZoomWidget.hpp>
#include <ui/MenuOverlay.hpp>
#include <ui/Slider.hpp>
#include <ui/MenuSeparator.hpp>
#include <ui/Button.hpp>
#include <ui/RadioButton.hpp>
#include <ui/OptionButton.hpp>
#include <ui/Tooltip.hpp>
#include <app/ModuleWidget.hpp>
#include <app/Scene.hpp>
#include <engine/Engine.hpp>
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
    settings::ModuleInfo& mi =
        settings::moduleInfos[model->plugin->slug][model->slug];
    mi.added++;
    mi.lastAdded = system::getUnixTime();

    history::ComplexAction* h = new history::ComplexAction;
    h->name = string::translate("Browser.history.addModule");

    // Create Module and ModuleWidget
    INFO("Creating module %s", model->getFullName().c_str());
    engine::Module* module = model->createModule();
    getEngine()->addModule(module);

    INFO("Creating module widget %s", model->getFullName().c_str());
    ModuleWidget* moduleWidget = model->createModuleWidget(module);

    getRack()->deselectAll();
    getRack()->updateModuleOldPositions();
    getRack()->addModuleAtMouse(moduleWidget);
    h->push(getRack()->getModuleDragAction());

    // Select the module so that it is obvious which module has been
    // added to the rack.
    getRack()->select(moduleWidget, true);

    // Load template preset
    moduleWidget->loadTemplate();

    // history::ModuleAdd
    history::ModuleAdd* ha = new history::ModuleAdd;
    // This serializes the module so redoing returns to the current state.
    ha->setModule(moduleWidget);
    h->push(ha);

    getHistory()->push(h);

    // Hide Module Browser since user has chosen a module
    getScene()->getBrowserOverlay()->hide();

    return moduleWidget;
}


/** 
 * For covering up everything outside the Browser Window and making
 * that area darker. This way the user focus is on the Browser Window
 * yet they still see that the rack window is there, right underneath.
 */
struct BrowserOverlay : ui::MenuOverlay {
    BrowserOverlay() {
        // To initially hide the Browser window need to actually
        // hide the BrowserOverlay.
        hideInitially();
    }
    
    void step() override {
        // Only step if visible, since there are potentially thousands of
        // descendants that don't need to be stepped.
        if (isVisible()) MenuOverlay::step();
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

    /** Updates the zoom level of this module */
	void updateZoom() {
        // Determine the fractional zoom level to use. If not VCV Rack, use standard choices.
        // But if VCV Rack, use powers of 2.
        float zoom = settings::isNotVCVRack
                         ? settings::browserZoom
                         : std::pow(2.f, settings::browserZoom);

        if (previewWidget) {
            fb->setDirty();
            zoomWidget->setZoom(zoom);
            setWidth(moduleWidget->getWidth() * zoom);
        } else {
            // Approximate size as 12HP before we know the actual size.
            // We need a nonzero size, otherwise too many ModelBoxes will lazily
            // render in the same frame.
            setWidth(12 * RACK_GRID_WIDTH * zoom);
        }
		setHeight(RACK_GRID_HEIGHT * zoom);
		setSize(getSize().ceil());
	}

	void createPreview() {
		if (previewWidget)
			return;

		previewWidget = new widget::TransparentWidget;
		addChild(previewWidget);

		zoomWidget = new widget::ZoomWidget;
		previewWidget->addChild(zoomWidget);

		fb = new widget::FramebufferWidget;
		if (getWindow()->pixelRatio_ < 2.0) {
			// Small details draw poorly at low DPI, so oversample when drawing to the framebuffer
			fb->setOversample(2.0);
		}
		zoomWidget->addChild(fb);

		mwc = new ModuleWidgetContainer;
		fb->addChild(mwc);

		INFO("Creating module widget %s", model->getFullName().c_str());
		moduleWidget = model->createModuleWidget(NULL);
		mwc->addChild(moduleWidget);
		mwc->setSize(moduleWidget->getSize());

		// Step ModuleWidget so it can set its default appearance.
		moduleWidget->step();

		updateZoom();
	}

    void draw(const DrawArgs& args) override {
        // Lazily create preview when drawn
        createPreview();

        // Draw shadow
        nvgBeginPath(args.vg);
        float r = 10;  // Blur radius
        float c = 5;   // Corner radius
        math::Rect shadowBox = getBox().zeroPos().grow(math::Vec(r, r));
        nvgRect(args.vg, RECT_ARGS(shadowBox));
        NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.5);
        nvgFillPaint(args.vg, nvgBoxGradient(args.vg, 0, 0, getWidth(),
                                                getHeight(), c, r, shadowColor,
                                                color::BLACK_TRANSPARENT));
        nvgFill(args.vg);

        // To avoid blinding the user when rack brightness is low, draw
        // framebuffer with the same brightness.
        float b = math::clamp(settings::rackBrightness + 0.2f, 0.f, 1.f);
        nvgGlobalTint(args.vg, nvgRGBAf(b, b, b, 1));

        OpaqueWidget::draw(args);

        // Draw favorite border if module has been favorited
        const settings::ModuleInfo* mi =
            settings::getModuleInfo(model->plugin->slug, model->slug);
        if (mi && mi->favorite) {
            nvgBeginPath(args.vg);
            math::Rect borderBox = getBox().zeroPos();
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
			getScene()->addChild(tooltip);
			this->tooltip = tooltip;
		}
	}

	void onButton(const ButtonEvent& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && (e.mods & RACK_MOD_MASK) == 0) {
			ModuleWidget* mw = chooseModel(model);

			// Pretend the moduleWidget was clicked so it can be dragged in the RackWidget
			e.consume(mw);

			// Set the drag position at the center of the module
			mw->dragOffset() = mw->getSize().div(2);
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

    /** Determines if the tagId is one that should be filtered out because it is
     * not really helpful info to the user. Specifically, filters out the "Hardware clone" tag.
     */
    static bool shouldFilterTag(int tagId) {
        int undesiredTagId = tag::findId("Hardware clone");
        return tagId == undesiredTagId;
    }

    /** Creates the tooltip text for a module in the Browser window. 
     * @return A new Tooltip instance with the created text.
    */
	ui::Tooltip* createTooltip() {
		std::string text;
        text += model->plugin->brand;
		text += " - ";
		text += model->name;
		
		// Description
		if (model->description != "") {
			text += "\n" + model->description;
		}

		// Tags (aka Types)
		text += "\n" + string::translate("Browser.tooltipTags");
		std::vector<std::string> tags;
		for (int tagId : model->tagIds) {
            // Filter out certain tags that are not helpful to show to the user
            if (shouldFilterTag(tagId))
                continue;

			std::string tag = string::translate("tag." + tag::getTag(tagId));
			tags.push_back(tag);
		}
		text += string::join(tags, ", ");

        // Create and return tooltip
		return new ui::Tooltip(text);
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


/**
 * BrowserHeader is the widget that is at the head of the Module Browser. It's only purpose
 * is to override SequentialLayout onDraw() so that a specified font can be used for all
 * of the children of the header. It uses the same layout algorithm as SequentialLayout.
 */
class BrowserHeader : public ui::SequentialLayout {
   public:
   /** Create browser header, and use center alignment and if two rows of children
    * make them even.
    */
    BrowserHeader() : SequentialLayout(CENTER_ALIGNMENT, true) {}

    /** Font size to use for all widgets in the header */
    static const int HEADER_WIDGETS_FONT_SIZE = 16;
    /** Height of all widgets in the header */
    static const int HEADER_WIDGET_HEIGHT = 24;

    /** Override draw() so that all children of the header are drawn with a specified font size.
     * This is necessary since the header contains buttons and labels that need to be drawn with a
     * smaller font size than the rest of the browser because they contain longer text strings.
     */
    void draw(const DrawArgs& args) override {
        // Temporarily store the current font size
        int originalFontSize = settings::getLabelFontSize();
        int originalWidgetHeight = settings::getWidgetHeight();

        // Set the font size for the widgets in theheader
        settings::setLabelFontSize(HEADER_WIDGETS_FONT_SIZE);
		settings::setWidgetHeight(BrowserHeader::HEADER_WIDGET_HEIGHT);

        // Draw all the children of the header using that font size
        SequentialLayout::draw(args);

        // Restore the font size
        settings::setLabelFontSize(originalFontSize);
        settings::setWidgetHeight(originalWidgetHeight);
    }
};

Browser::Browser() {
    // Browser big title label at the top
    titleLabel_ = new ui::Label(string::translate("Browser.title"));
    titleLabel_->setFontSize(40);

    // Set color of title to contrast with background
    titleLabel_->setColor(color::BLACK);
    titleLabel_->setAlignment(ui::Label::Alignment::CENTER_ALIGNMENT);

    titleLabel_->setX(MARGIN_);
    titleLabel_->setY(0);

    // Don't need full font height for label
    titleLabel_->setHeight(titleLabel_->getFontSize() * 0.7 + 2.0);
    addChild(titleLabel_);

    // Create header below the label. It contains the selectors.
    headerLayout_ = new BrowserHeader();
    headerLayout_->setPos(
        math::Vec(0, titleLabel_->getBox().getBottom() + MARGIN_ + 4));
    headerLayout_->setMargin(math::Vec(MARGIN_, 0));
    headerLayout_->setMinSpacing(math::Vec(MARGIN_, MARGIN_));
    addChild(headerLayout_);

    // Need to set desired widgetHeight here instead of when drawing since
    // box.size.y is set in Button's constructor
    int originalWidgetHeight = settings::getWidgetHeight();
    settings::setWidgetHeight(BrowserHeader::HEADER_WIDGET_HEIGHT);

    // Note: for the header widgets size.x using original values for VCV Rack,
    // but then adjusting them to the font size actually being used here.
    // This way it is easy to change the font size simply by setting
    // HeaderFontSize.

    searchField_ = new BrowserSearchField(*this);
    searchField_->setWidth(
        150 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    searchField_->setFontSize(BrowserHeader::HEADER_WIDGETS_FONT_SIZE);
    searchField_->setPlaceholder(string::translate("Browser.searchModules"));
    headerLayout_->addChild(searchField_);

    brandButton_ = new BrandButton(*this);
    brandButton_->setWidth(
        150 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(brandButton_);

    tagButton_ = new TagButton(*this);
    tagButton_->setWidth(
        150 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(tagButton_);

    favoriteButton_ = new FavoriteButton(*this);
    favoriteButton_->setWidth(
        110 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(favoriteButton_);

    clearButton_ = new ClearButton(*this);
    clearButton_->setWidth(
        120 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(clearButton_);

    // Can already see the modules. And it takes up precious space. Plus there
    // are already lots of other action widgets, complicating the UI. Therefore
    // don't display it unless VCV Rack where want UI to be consistent.
    if (!settings::isNotVCVRack) {
        countLabel_ = new ui::Label;
        countLabel_->setWidth(
            110 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
        headerLayout_->addChild(countLabel_);
    }

    SortButton* sortButton = new SortButton(*this);
    sortButton->setWidth(
        140 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(sortButton);

    // For zooming in or out on the modules in the Browser
    ZoomButton* zoomButton = new ZoomButton(*this);
    zoomButton->setWidth(
        140 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(zoomButton);

    // For adding modules from VCV Rack to the users' library
    UrlButton* libraryButton =
        new UrlButton("https://library.vcvrack.com/",
                      string::translate("Browser.browseLibrary"));
    libraryButton->setWidth(
        170 * BrowserHeader::HEADER_WIDGETS_FONT_SIZE / BND_LABEL_FONT_SIZE);
    headerLayout_->addChild(libraryButton);

    // Restore original widget height since done creating header widgets
    settings::setWidgetHeight(originalWidgetHeight);

    // Create scrollable module container
    moduleScroll_ = new ui::ScrollWidget();
    addChild(moduleScroll_);

    // Make scrolled area a bit darker than the main background
    moduleScroll_->setColors(
        color::lerp(settings::moduleBrowserBg, color::BLACK, 0.2), 
        color::BLACK);

    moduleMargin_ = new widget::Widget();
    moduleScroll_->container->addChild(moduleMargin_);

    const float MIN_HORIZONTAL_SPACING = 2 * MARGIN_;;
    moduleLayoutContainer_ =
        new ui::SequentialLayout(ui::SequentialLayout::TAKE_ENTIRE_WIDTH);
    moduleLayoutContainer_->setMargin(
        math::Vec(MIN_HORIZONTAL_SPACING, MARGIN_)); 
    moduleLayoutContainer_->setMinSpacing(math::Vec(MIN_HORIZONTAL_SPACING, MARGIN_));
    moduleMargin_->addChild(moduleLayoutContainer_);

    resetModuleBoxes();
    clearSelectorsInHeader();
}

void Browser::resetModuleBoxes() {
    moduleLayoutContainer_->clearChildren();
    modelOrders_.clear();
    // Iterate plugins
    for (plugin::Plugin* plugin : plugin::plugins) {
        // Iterate models in plugin
        int modelIndex = 0;
        for (plugin::Model* model : plugin->models) {
            // Create ModelBox
            ModelBox* modelBox = new ModelBox;
            modelBox->setModel(model);
            moduleLayoutContainer_->addChild(modelBox);

            modelOrders_[model] = modelIndex;
            modelIndex++;
        }
    }
}

void Browser::updateZoom() {
    moduleScroll_->offset = math::Vec();

    for (Widget* w : moduleLayoutContainer_->getChildren()) {
        ModelBox* mb = reinterpret_cast<ModelBox*>(w);
        assert(mb);
        mb->updateZoom();
    }
}

void Browser::step() {
    // Determine size of the Browser window. Make it 40 units smaller than
    // the main window so that can see that the window is on top of the rack
    // display
    setBox(getParent()->getBox().zeroPos().grow(math::Vec(-36, -36)));

    // Determine horizontal layout of titleLabel
    titleLabel_->setWidth(getWidth());

    // The modules to edges of window margin
    const float rightAndBottomMargin = MARGIN_;

    // Now that know how big enclosing window is can set position and sizes
    // of the containers. First, set width of the headerLayout widget.
    headerLayout_->setWidth(getWidth());

    // Set the position and size of the scrollable container that contains
    // all the modules. Make it so there is a margin at the bottom.
    const float EDGE_MARGIN = 5.0f;
    moduleScroll_->setPos(headerLayout_->getBox().getBottomLeft() +
                          math::Vec(EDGE_MARGIN, MARGIN_));
    moduleScroll_->setSize(getSize().minus(moduleScroll_->getPos()) -
                           math::Vec(EDGE_MARGIN, MARGIN_ / 2));

    // Set the size of the moduleMargin which is the usable inside  area of 
    // the moduleScroll
    moduleMargin_->setSize(
        moduleScroll_->getWidth(),
        moduleLayoutContainer_->getHeight() + rightAndBottomMargin);
    moduleLayoutContainer_->setWidth(
        moduleMargin_->getWidth() - rightAndBottomMargin);

    // Check if preferDarkPanels has changed
    if (settings::preferDarkPanels != lastPreferDarkPanels_) {
        lastPreferDarkPanels_ = settings::preferDarkPanels;
        // Request module framebuffers to re-render
        Widget::DirtyEvent eDirty;
        moduleLayoutContainer_->onDirty(eDirty);
    }

    OpaqueWidget::step();
}

void Browser::draw(const DrawArgs& args) {
    // Draw a light background
    NVGcolor bg_color = settings::moduleBrowserBg;
    // Outline color that contrasts with the background color
    NVGcolor outline_color =
        color::brightness(bg_color) < 0.5f
            ? color::lerp(bg_color, color::WHITE,
                          0.1)  // Light outline for dark background
            : color::lerp(bg_color, color::BLACK,
                          0.1);  // Dark outline for light background
    float radius = 10.0;  // Use noticeable radius to differentiate window
    bndBackgroundColor(args.vg, 0.0, 0.0, getWidth(), getHeight(), radius,
                       bg_color, outline_color);

    Widget::draw(args);
}

bool Browser::isModelVisible(plugin::Model* model, const std::string& brand,
                             std::set<int> tagIds, bool favorite) {
    // Filter hidden
    if (model->hidden) return false;

    // Filter moduleInfo setting if enabled is false
    settings::ModuleInfo* mi =
        settings::getModuleInfo(model->plugin->slug, model->slug);
    if (mi && !mi->enabled) return false;

    // Filter if not whitelisted by library
    if (!settings::isModuleWhitelisted(model->plugin->slug, model->slug))
        return false;

    // Filter favorites
    if (favorite) {
        if (!mi || !mi->favorite) return false;
    }

    // Filter brand
    if (!brand.empty()) {
        if (model->plugin->brand != brand) return false;
    }

    // Filter tag
    for (int tagId : tagIds) {
        auto it = std::find(model->tagIds.begin(), model->tagIds.end(), tagId);
        if (it == model->tagIds.end()) return false;
    }

    return true;
};

bool Browser::hasVisibleModel(const std::string& brand, std::set<int> tagIds,
                              bool favoritesEnabled) {
    for (const auto& pair : prefilteredModelScores_) {
        plugin::Model* model = pair.first;
        if (isModelVisible(model, brand, tagIds, favoritesEnabled)) return true;
    }
    return false;
};

template <typename F>
void Browser::sortModels(F f) {
    moduleLayoutContainer_->getChildren().sort([&](Widget* w1, Widget* w2) {
        ModelBox* m1 = reinterpret_cast<ModelBox*>(w1);
        ModelBox* m2 = reinterpret_cast<ModelBox*>(w2);
        return f(m1) < f(m2);
    });
}

void Browser::refresh() {
    // Reset scroll position
    moduleScroll_->offset = math::Vec();

    prefilteredModelScores_.clear();

    // Filter ModelBoxes by brand and tag
    for (Widget* w : moduleLayoutContainer_->getChildren()) {
        ModelBox* m = reinterpret_cast<ModelBox*>(w);
        m->setVisible(isModelVisible(m->model, brand_, tagIds_,
                                     favoriteButton_->isEnabled()));
    }

    // Filter and sort by search results
    if (search_.empty()) {
        // Add all models to prefilteredModelScores with scores of 1
        for (Widget* w : moduleLayoutContainer_->getChildren()) {
            ModelBox* m = reinterpret_cast<ModelBox*>(w);
            prefilteredModelScores_[m->model] = 1.f;
        }

        // Sort ModelBoxes
        if (settings::browserSort == settings::BROWSER_SORT_UPDATED) {
            sortModels([this](ModelBox* m) {
                plugin::Plugin* p = m->model->plugin;
                int modelOrder = get(modelOrders_, m->model, 0);
                return std::make_tuple(-p->modifiedTimestamp, p->brand, p->name,
                                       modelOrder);
            });
        } else if (settings::browserSort == settings::BROWSER_SORT_LAST_USED) {
            sortModels([this](ModelBox* m) {
                plugin::Plugin* p = m->model->plugin;
                const settings::ModuleInfo* mi =
                    settings::getModuleInfo(p->slug, m->model->slug);
                double lastAdded = mi ? mi->lastAdded : -INFINITY;
                int modelOrder = get(modelOrders_, m->model, 0);
                return std::make_tuple(-lastAdded, -p->modifiedTimestamp,
                                       p->brand, p->name, modelOrder);
            });
        } else if (settings::browserSort == settings::BROWSER_SORT_MOST_USED) {
            sortModels([this](ModelBox* m) {
                plugin::Plugin* p = m->model->plugin;
                const settings::ModuleInfo* mi =
                    settings::getModuleInfo(p->slug, m->model->slug);
                int added = mi ? mi->added : 0;
                double lastAdded = mi ? mi->lastAdded : -INFINITY;
                int modelOrder = get(modelOrders_, m->model, 0);
                return std::make_tuple(-added, -lastAdded,
                                       -p->modifiedTimestamp, p->brand, p->name,
                                       modelOrder);
            });
        } else if (settings::browserSort == settings::BROWSER_SORT_BRAND) {
            sortModels([this](ModelBox* m) {
                plugin::Plugin* p = m->model->plugin;
                int modelOrder = get(modelOrders_, m->model, 0);
                return std::make_tuple(p->brand, p->name, modelOrder);
            });
        } else if (settings::browserSort == settings::BROWSER_SORT_NAME) {
            sortModels([](ModelBox* m) {
                plugin::Plugin* p = m->model->plugin;
                return std::make_tuple(m->model->name, p->brand);
            });
        } else if (settings::browserSort == settings::BROWSER_SORT_RANDOM) {
            std::map<ModelBox*, uint64_t> randomOrder;
            for (Widget* w : moduleLayoutContainer_->getChildren()) {
                ModelBox* m = reinterpret_cast<ModelBox*>(w);
                randomOrder[m] = random::u64();
            }
            sortModels([&](ModelBox* m) { return get(randomOrder, m, 0); });
        }
    } else {
        // Score results against search query
        auto results = modelDb.search(search_);
        // DEBUG("=============");
        for (auto& result : results) {
            prefilteredModelScores_[result.key] = result.score;
            // DEBUG("%s %s\t\t%f", result.key->plugin->slug.c_str(),
            // result.key->slug.c_str(), result.score);
        }
        // Sort by score
        sortModels([&](ModelBox* m) {
            return -get(prefilteredModelScores_, m->model, 0.f);
        });
        // Filter by whether the score is above the threshold
        for (Widget* w : moduleLayoutContainer_->getChildren()) {
            ModelBox* m = reinterpret_cast<ModelBox*>(w);
            assert(m);
            if (m->isVisible()) {
                if (prefilteredModelScores_.find(m->model) ==
                    prefilteredModelScores_.end())
                    m->hide();
            }
        }
    }

    // Only update countLabel if it was actually created
    if (countLabel_) {
        // Count visible modules
        int count = 0;
        for (Widget* w : moduleLayoutContainer_->getChildren()) {
            if (w->isVisible()) count++;
        }
        countLabel_->setText(
            (count == 1)
                ? string::translate("Browser.modulesOne")
                : string::f(string::translate("Browser.modulesMany"), count));
    }
}

void Browser::clearSelectorsInHeader() {
    search_ = "";
    searchField_->setText("");
    brand_ = "";
    tagIds_ = {};
    favoriteButton_->disable();
    refresh();
}

void Browser::onButton(const ButtonEvent& e) {
    Widget::onButton(e);
    e.stopPropagating();
    // Consume all mouse buttons
    if (!e.isConsumed()) e.consume(this);
}

void Browser::onHoverKey(const HoverKeyEvent& e) {
    if (e.action == GLFW_PRESS) {
        // Secret key command to dump all visible modules into rack
        if (e.isKeyCommand(GLFW_KEY_F2,
                           RACK_MOD_CTRL | GLFW_MOD_SHIFT | GLFW_MOD_ALT)) {
            int count = 0;
            for (widget::Widget* w : moduleLayoutContainer_->getChildren()) {
                ModelBox* mb = dynamic_cast<ModelBox*>(w);
                if (!mb) continue;
                if (!mb->isVisible()) continue;
                count++;
                DEBUG("Dumping into rack (%d): %s/%s", count,
                      mb->model->plugin->slug.c_str(), mb->model->slug.c_str());
                chooseModel(mb->model);
            }
            e.consume(this);
        }
    }

    if (e.isConsumed()) return;
    OpaqueWidget::onHoverKey(e);
}

void Browser::FavoriteButton::onAction(const ActionEvent& e) {
    // Toggle enabled state
    enabled = !enabled;

    // Set the checkmark on right side of button if now enabled
    std::string label = string::translate("Browser.favorites");
    if (enabled) {
        label += "  ";
        label += CHECKMARK_STRING;
    }
    setText(label);

    // Redisplay everything
    browser.refresh();
}


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

void Browser::SortButton::step() {
    text_ = string::translate("Browser.sort");
    text_ += getSortNames()[settings::browserSort];
    text_ = string::ellipsize(text_, 20);
    ChoiceButton::step();
}

void Browser::ZoomButton::step() {
    // Determine the current zoom level to display
    text_ = string::translate("Browser.zoom");
    float zoom_fraction = settings::isNotVCVRack
                              ? settings::browserZoom
                              : std::pow(2.f, settings::browserZoom);
    text_ += string::f("%.0f%%", zoom_fraction * 100.f);
    ChoiceButton::step();
}

void Browser::UrlButton::onAction(const ActionEvent& e) {
    system::openBrowser(url);
}

// Implementations to resolve dependencies


void Browser::ClearButton::onAction(const ActionEvent& e) {
	browser.clearSelectorsInHeader();
}

void Browser::BrowserSearchField::onSelectKey(const SelectKeyEvent& e) {
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		// Backspace when the field is empty to clear filters.
		if (e.isKeyCommand(GLFW_KEY_BACKSPACE) || e.isKeyCommand(GLFW_KEY_BACKSPACE, RACK_MOD_CTRL)) {
			if (getText() == "") {
				browser.clearSelectorsInHeader();
				e.consume(this);
			}
		}
	}

	if (!e.getTarget())
		ui::TextField::onSelectKey(e);
}

void Browser::BrowserSearchField::onChange(const ChangeEvent& e) {
    browser.setSearch(string::trim(getText()));
    browser.refresh();
}

void Browser::BrowserSearchField::onAction(const ActionEvent& e) {
	// Get first ModelBox
	ModelBox* mb = NULL;
	for (Widget* w : browser.getModuleLayoutContainer()->getChildren()) {
		if (w->isVisible()) {
			mb = reinterpret_cast<ModelBox*>(w);
			break;
		}
	}

	if (mb) {
		chooseModel(mb->model);
	}
}

void Browser::BrandItem::onAction(const ActionEvent& e) {
	if (browser.getBrand() == getText()) {
        // Set to all brands
		browser.setBrand("");
    } else {
        // Set to this brand
		browser.setBrand(getText());
    }
	browser.refresh();
}

void Browser::BrandItem::step() {
    setRightText(CHECKMARK(browser.getBrand() == getText()));
    MenuItem::step();
}

void Browser::BrandButton::onAction(const ActionEvent& e) {
    ui::Menu* menu = createMenu();
    menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));
    menu->setWidth(getWidth());

    BrandItem* noneSelectedItem =
        new BrandItem(browser, string::translate("Browser.allBrands"));
    menu->addChild(noneSelectedItem);

    menu->addChild(new ui::MenuSeparator);

    // Collect brands from all plugins
    std::set<std::string, string::CaseInsensitiveCompare> brands;
    for (plugin::Plugin* plugin : plugin::plugins) {
        brands.insert(plugin->brand);
    }

    for (const std::string& brand : brands) {
        BrandItem* brandItem = new BrandItem(browser, brand);
        brandItem->setDisabled(!browser.hasVisibleModel(
            brand, browser.getTagIds(), browser.getFavoriteButton()->isEnabled()));
        menu->addChild(brandItem);
    }
}

void Browser::BrandButton::step() {
	text_ = string::translate("Browser.brand");
    if (!browser.getBrand().empty()) {
        text_ += ": ";
        text_ += browser.getBrand();
    }
	text_ = string::ellipsize(text_, 20);
	ChoiceButton::step();
}

/** Called when user clicks on an item in the tag menu */
void Browser::TagItem::onAction(const ActionEvent& e) {
	auto it = browser.getTagIds().find(tagId);
	bool isSelected = (it != browser.getTagIds().end());

	if (tagId >= 0) {
		// Specific tag
		if (!e.isConsumed()) {
			// Multi select
			if (isSelected)
				browser.getTagIds().erase(tagId);
			else
				browser.getTagIds().insert(tagId);
			e.unconsume();
		}
		else {
			// Single select
			if (isSelected)
				browser.setTagIds({});
			else {
				browser.setTagIds({tagId});
			}
		}
	}
	else {
		// All tags
		browser.setTagIds({});
	}

	browser.refresh();
}

void Browser::TagItem::step() {
	// TODO Disable tags with no modules
	if (tagId >= 0) {
		auto it = browser.getTagIds().find(tagId);
		bool isSelected = (it != browser.getTagIds().end());
		setRightText(CHECKMARK(isSelected));
	}
	else {
		setRightText(CHECKMARK(browser.getTagIds().empty()));
	}
	MenuItem::step();
}

void Browser::TagButton::onAction(const ActionEvent& e) {
    ui::Menu* menu = createMenu();
    menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));
    menu->setWidth(getWidth());

    // So user can select no tags/types
    TagItem* noneItem = new TagItem(browser);
    noneItem->setText(string::translate("Browser.allTags"));
    menu->addChild(noneItem);

    if (settings::hasTouchscreen) {
        // Touchscreen, so Let user know they can select multiple tags by long
        // click
        menu->addChild(createMenuLabel(
            string::translate("Browser.tagsSelectMultipleTouchScreen")));
    } else {
        // Not a touch screen so tell user they can use MOD_CTRL click to select
        // multiple
        menu->addChild(
            createMenuLabel(widget::getKeyCommandName(0, RACK_MOD_CTRL) +
                            string::translate("key.click") +
                            string::translate("Browser.tagsSelectMultiple")));
    }
    menu->addChild(new ui::MenuSeparator);

    for (int tagId = 0; tagId < (int)tag::tagAliases.size(); tagId++) {
        TagItem* tagItem = new TagItem(browser, tagId);
        tagItem->setText(string::translate("tag." + tag::getTag(tagId)));
        tagItem->setDisabled(!browser.hasVisibleModel(
            browser.getBrand(), {tagId}, browser.getFavoriteButton()->isEnabled()));
        menu->addChild(tagItem);
    }
}

void Browser::TagButton::step() {
	text_ = string::translate("Browser.tags");
	if (!browser.getTagIds().empty()) {
		text_ += ": ";
		bool firstTag = true;
		for (int tagId : browser.getTagIds()) {
			if (!firstTag)
				text_ += ", ";
			std::string tag = string::translate("tag." + tag::getTag(tagId));
			text_ += tag;
			firstTag = false;
		}
	}
	text_ = string::ellipsize(text_, 20);
	ChoiceButton::step();
}

void Browser::SortButton::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
    menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));
    menu->setWidth(getWidth());

	for (int sortId = 0; sortId <= settings::BROWSER_SORT_RANDOM; sortId++) {
		menu->addChild(createCheckMenuItem(getSortNames()[sortId], "",
			[=]() {return settings::browserSort == sortId;},
			[=]() {
				settings::browserSort = (settings::BrowserSort) sortId;
				browser.refresh();
			}
		));
	}
}

/** Called when user clicks on the zoom button. Shows possible choices. */
void Browser::ZoomButton::onAction(const ActionEvent& e) {
	ui::Menu* menu = createMenu();
    menu->setPos(getInSceneCoords(math::Vec(0, getHeight())));
    menu->setWidth(getWidth());

    if (!settings::isNotVCVRack) {
        // Use standard VCV Rack zoom level choices
        for (float zoom = 1.f; zoom >= -2.f; zoom -= 0.5f) {
            menu->addChild(createCheckMenuItem(
                string::f("%.0f%%", std::pow(2.f, zoom) * 100.f), "",
                [=]() { return settings::browserZoom == zoom; },
                [=]() {
                    if (zoom == settings::browserZoom) return;
                    settings::browserZoom = zoom;
                    browser.updateZoom();
                }));
        }
    } else {
        // Not VCV Rack so use choices that don't appear to be mystical numbers.
        // Simply don't make the user get distracted thinking about the choices.
        std::vector<float> zoomLevels = {2.0f, 1.5f, 1.0f, 0.75f, 0.5f, 0.25f};
        for (float zoom : zoomLevels) {
            menu->addChild(createCheckMenuItem(
                string::f("%.0f%%", zoom * 100.f), "",
                [=]() { return settings::browserZoom == zoom; },
                [=]() {
                    if (zoom == settings::browserZoom) return;
                    settings::browserZoom = zoom;
                    browser.updateZoom();
                }));
        }
    }
}


void browserInit() {
	browser::modelDbInit();
}


Browser* browserCreate() {
    // Draw a dark area over the rest of the UI. This way user's focus is on
    // the Browser Window but they can still see that the Rack window is there,
    // underneath.
    browser::BrowserOverlay* overlay = new browser::BrowserOverlay;
    // Set opacity for where drawing on top of the rack. Higher the value the
    // darker things get
    overlay->bgColor = nvgRGBAf(0, 0, 0, 0.58);

    // Now actually create the Browser window and add it to the overlay heirachy
	Browser* browser = new Browser();
	overlay->addChild(browser);

	return browser;
}

} // namespace browser
} // namespace app
} // namespace rack
