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
namespace browser {

/** Global static function that initializes the browser */
PRIVATE void browserInit();

class Browser; // Forward declaration
/** Global static function that creates the browser window and returns it. 
 * The Browser has an overlay widget. If need access to the overlay, such as 
 * to show or hide it, then use getParent().
 */
PRIVATE Browser* browserCreate();


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
        BrowserSearchField(Browser& browser) : browser_(browser) {}

       private:
        Browser& browser_;

        void step() override {
            // Steal focus when step is called
            getEvent()->setSelectedWidget(this);
            TextField::step();
        }

        void onSelectKey(const SelectKeyEvent& e) override;
        void onChange(const ChangeEvent& e) override;
        void onAction(const ActionEvent& e) override;

        void onHide(const HideEvent& e) override {
            getEvent()->setSelectedWidget(NULL);
            ui::TextField::onHide(e);
        }

        void onShow(const ShowEvent& e) override {
            selectAll();
            TextField::onShow(e);
        }
    };

    class BrandButton : public ui::ChoiceButton {
       public:
        BrandButton(Browser& browser) : browser_(browser) {}

       private:
        Browser& browser_;

        /** Called when user clicks on Brand button so popup brand menu */
        void onAction(const ActionEvent& e) override;

        void step() override;
    };

    /** Button for choosing types/tags */
    class TagButton : public ui::ChoiceButton {
       public:
        TagButton(Browser& browser) : browser_(browser) {}

       private:
        Browser& browser_;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    /** An item for the tag menu. Causes the TagItem::onAction() method to be
     * called on a click. */
    class TagItem : public ui::MenuItem {
       public:
        TagItem(Browser& browser, int tagId = -1)
            : browser_(browser), tagId_(tagId) {}

       private:
        Browser& browser_;
        int tagId_;

        /** Called when user clicks on an item in the tag/type menu */
        void onAction(const ActionEvent& e) override;

        void step() override;
    };

    /** Button for enabling/disabling showing of only modules that are favorites
     */
    class FavoriteButton : public ui::Button {
       public:
        FavoriteButton(Browser& browser)
            : Button(string::translate("Browser.favorites")),
              browser_(browser) {}

        bool isEnabled() {
            return enabled_;
        }

        void disable() {
            this->enabled_ = false;
        }

       private:
        Browser& browser_;

        bool enabled_ = false;

       public:
        void onAction(const ActionEvent& e) override;
    };

    /** Button for resetting/clearing all filters */
    class ClearButton : public ui::Button {
       public:
        ClearButton(Browser& browser)
            : Button(string::translate("Browser.resetFilters")),
              browser_(browser) {}

       private:
        Browser& browser_;

        void onAction(const ActionEvent& e) override;
    };

    class BrandItem : public ui::MenuItem {
       public:
        BrandItem(Browser& browser, const std::string& brand = "")
            : MenuItem(brand), browser_(browser) {}

       private:
        Browser& browser_;

        /** Called when user clicks on an item in the brand menu */
        void onAction(const ActionEvent& e) override;

        void step() override;
    };

    class SortButton : public ui::ChoiceButton {
       public:
        SortButton(Browser& browser) : browser_(browser) {}

       private:
        Browser& browser_;

        void onAction(const ActionEvent& e) override;
        void step() override;
    };

    /** Zoom selector for the browser */
    class ZoomButton : public ui::ChoiceButton {
       public:
        ZoomButton(Browser& browser) : browser_(browser) {}

       private:
        Browser& browser_;

        // Shows the choices
        void onAction(const ActionEvent& e) override;

        void step() override;
    };

    class UrlButton : public ui::Button {
       public:
        UrlButton(const std::string& url, const std::string& text)
            : Button(text), url_(url) {}

       private:
        std::string url_;

        void onAction(const ActionEvent& e) override;
    };
    // End of internal classes for Browser

    // The start of the actual Browser class
   public:
    Browser();

    // These functions are used externally so must be public
    void refresh();
    void clearSelectorsInHeader();
    bool hasVisibleModel(const std::string& brand, std::set<int> tagIds, bool favoritesEnabled);

    /** Called when user selects zoom level. Updates zoom for each module in
     * the Browser */
    void updateZoom();

    void setSearch(const std::string& searchStr) {
        search_ = searchStr;
    }

    /** Sets brand selected for the Browser window */
    void setBrand(const std::string& brand) {
        this->brand_ = brand;
    }

    /** Returns the brand selected for the Browser window */
    std::string getBrand() const {
        return brand_;
    }

    FavoriteButton* getFavoriteButton() {
        return favoriteButton_;
    }

    std::set<int>& getTagIds() {
        return tagIds_;
    }

    void setTagIds(const std::set<int>& tagIds) {
        this->tagIds_ = tagIds;
    }

    ui::SequentialLayout* getModuleLayoutContainer() const {
        return moduleLayoutContainer_;
    }

    /** Updates the list of plugins in the browser. To be called when the global
     * plugins list is changed (like after loading a new plugin).
     */
    void updateBrowserPlugins() {
        resetModuleBoxes();

        // Need to update the module list
        refresh();
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

    ui::Label* titleLabel_;
    ui::SequentialLayout* headerLayout_;
    BrowserSearchField* searchField_;
    BrandButton* brandButton_;
    TagButton* tagButton_;
    FavoriteButton* favoriteButton_;
    ClearButton* clearButton_;
    ui::Label* countLabel_ = nullptr; // Nulled here because not always created

    ui::ScrollWidget* moduleScroll_;
    widget::Widget* moduleMargin_;
    ui::SequentialLayout* moduleLayoutContainer_;

    std::string search_;
    std::string brand_;
    std::set<int> tagIds_ = {};
    bool lastPreferDarkPanels_ = false;

    // Caches and temporary state
    std::map<plugin::Model*, float> prefilteredModelScores_;
    std::map<plugin::Model*, int> modelOrders_;

    // Margin used for the small border around the Browser window
    const float MARGIN_ = 10.0;
}; // end of class Browser


} // namespace browser
} // namespace app
} // namespace rack
