/** 
 * The Browser is the window that shows the modules the user has
 * available and can add to their rack.
 */

#pragma once
#include <app/common.hpp>
#include <widget/Widget.hpp>
#include <widget/OpaqueWidget.hpp>
#include <plugin/Model.hpp>
#include <ui/ChoiceButton.hpp>
#include <ui/Label.hpp>
#include <ui/MenuItem.hpp>
#include <ui/SequentialLayout.hpp>
#include <ui/ScrollWidget.hpp>
#include <ui/TextField.hpp>
#include <context.hpp>
#include <plugin.hpp>

namespace rack {
namespace app {

/** Initializes the browser */
PRIVATE void browserInit();

/** Creates the browser window */
PRIVATE widget::Widget* browserCreate();


namespace browser {

/** The actual Browser class. The Browser is the window that shows the modules 
 * the user has available and can add to their rack.
 */
class Browser : public widget::OpaqueWidget {
    // First, defining internal classes so that they can be used by Browser.
    // By making them internal classes the circular dependency is avoided. And
    // They are hidden from the outside world.
   private:
    class BrowserSearchField : public ui::TextField {
       public:
        BrowserSearchField(Browser& browser) : browser(browser) {}

       private:
        Browser& browser;

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

    class BrandButton : public ui::ChoiceButton {
       public:
        BrandButton(Browser& browser) : browser(browser) {}

       private:
        Browser& browser;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    class TagButton : public ui::ChoiceButton {
       public:
        TagButton(Browser& browser) : browser(browser) {}

       private:
        Browser& browser;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    /** An item for the tag menu. Causes the TagItem::onAction() method to be
     * called on a click. */
    class TagItem : public ui::MenuItem {
       public:
        TagItem(Browser& browser, int tagId = -1)
            : browser(browser), tagId(tagId) {}

       private:
        Browser& browser;
        int tagId;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    /** Button for enabling/disabling showing of only modules that are favorites
     */
    class FavoriteButton : public ui::Button {
       public:
        FavoriteButton(Browser& browser)
            : Button(string::translate("Browser.favorites")),
              browser(browser) {}

        bool isEnabled() {
            return enabled;
        }

        void disable() {
            this->enabled = false;
        }

       private:
        Browser& browser;

        bool enabled = false;

       public:
        void onAction(const ActionEvent& e) override;
    };

    /** Button for resetting/clearing all filters */
    class ClearButton : public ui::Button {
       public:
        ClearButton(Browser& browser)
            : Button(string::translate("Browser.resetFilters")),
              browser(browser) {}

       private:
        Browser& browser;

        void onAction(const ActionEvent& e) override;
    };

    class BrandItem : public ui::MenuItem {
       public:
        BrandItem(Browser& browser, const std::string& brand = "")
            : MenuItem(brand), browser(browser) {}

       private:
        Browser& browser;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    class SortButton : public ui::ChoiceButton {
       public:
        SortButton(Browser& browser) : browser(browser) {}

       private:
        Browser& browser;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    /** Zoom selector for the browser */
    class ZoomButton : public ui::ChoiceButton {
       public:
        ZoomButton(Browser& browser) : browser(browser) {}

       private:
        Browser& browser;

        // Shows the choices
        void onAction(const ActionEvent& e) override;

        void step() override;
    };

    class UrlButton : public ui::Button {
       public:
        UrlButton(const std::string& url, const std::string& text)
            : Button(text), url(url) {}

       private:
        std::string url;

        void onAction(const ActionEvent& e) override;
    };
    // End of internal classes for Browser

    // The start of the actual Browser class
   public:
    Browser();

    // These functions are used exxternally so must be public
    void refresh();
    void clearSelectorsInHeader();
    bool hasVisibleModel(const std::string& brand, std::set<int> tagIds, bool favoritesEnabled);

    /** Called when user selects zoom level. Updates zoom for each module in
     * the Browser */
    void updateZoom();

    void setSearch(const std::string& searchStr) {
        search = searchStr;
    }

    void setBrand(const std::string& brand) {
        this->brand = brand;
    }

    std::string getBrand() const {
        return brand;
    }

    FavoriteButton* getFavoriteButton() {
        return favoriteButton;
    }

    std::set<int>& getTagIds() {
        return tagIds;
    }

    void setTagIds(const std::set<int>& tagIds) {
        this->tagIds = tagIds;
    }

    ui::SequentialLayout* getModuleLayoutContainer() const {
        return moduleLayoutContainer;
    }

    // The following are only used internally so are private
   private:
    void resetModuleBoxes();

    template <typename F>
    void sortModels(F f);

    void draw(const DrawArgs& args) override;
    void step() override;
    bool isModelVisible(plugin::Model* model, const std::string& brand, std::set<int> tagIds, bool favorite);
    void onButton(const ButtonEvent& e) override;
    void onHoverKey(const HoverKeyEvent& e) override;

    ui::Label* titleLabel;
    ui::SequentialLayout* headerLayout;
    BrowserSearchField* searchField;
    BrandButton* brandButton;
    TagButton* tagButton;
    FavoriteButton* favoriteButton;
    ClearButton* clearButton;
    ui::Label* countLabel;

    ui::ScrollWidget* moduleScroll;
    widget::Widget* moduleMargin;
    ui::SequentialLayout* moduleLayoutContainer;

    std::string search;
    std::string brand;
    std::set<int> tagIds = {};
    bool lastPreferDarkPanels = false;

    // Caches and temporary state
    std::map<plugin::Model*, float> prefilteredModelScores;
    std::map<plugin::Model*, int> modelOrders;

    // Margin used for the small border around the Browser window
    const float MARGIN = 8.0;
}; // end of class Browser

} // namespace browser
} // namespace app
} // namespace rack
