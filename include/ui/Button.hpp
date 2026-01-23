#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>
#include <Quantity.hpp>
#include <settings.hpp>


namespace rack {
namespace ui {

/** A clickable button with text.
Dispatches Action event when clicked.
If quantity is set, its value is set to 1.0 when pressed, 0.0 when released.
If text is not set, the quantity label is used.
*/
class Button : public widget::OpaqueWidget {
   public:
    /** Constructor with param for initialing text to a value. */
    Button(const std::string& text, const std::string& name = "")
        : OpaqueWidget(name) {
        setText(text);

        // Do other initialization that default constructor does
        setHeight(settings::bndWidgetHeight);
    }

    /** Need default constructor since module libraries might have used it, like
     * for 4ms. Having default values for constructor parameters does not count
     * as a default constructor.
     */
    Button();

    void setText(const std::string& text) {
        this->text_ = text;
    }

    void setQuantity(Quantity* quantity) {
        this->quantity_ = quantity;
    }

    Quantity* getQuantity() {
        return quantity_;
    }

   protected:
    /** The text label of the button. If empty, the quantity label is used. */
    std::string text_;

    /** Not owned. Tracks the pressed state of the button.*/
    Quantity* quantity_ = nullptr;

   protected:
    /** Handles button event, whether press or release, left click or whatever
     */
    void onButton(const ButtonEvent& event) override;

    /** Handles double-click event. Cancels any pending action events since
     * double-click supersedes them
     */
    void onDoubleClick(const DoubleClickEvent& event) override;

    /** Handles action events triggered by the button. Sets the quantity to
     * proper value 
     */
    void onAction(const ActionEvent& event) override;

   private:
    /** Draws the button based on its state of default, hovered, or being
     * dragged */
    void draw(const DrawArgs& args) override;

    /** Triggers the action event for the button, either immediately or delayed.
     * The delay is for if a button press should wait to see if superseded by
     * another event, such as a double-click.
     */
    void triggerActionEvent(const ButtonEvent& buttonEvent);

    //void onDragStart(const DragStartEvent& e) override;
    /** @deprecated only reason kept around is because if modules
     * by 3rd parties used Button then they need to access this
     * function during linking.
     */
    void onDragEnd(const DragEndEvent& e) override;
    void onDragDrop(const DragDropEvent& e) override;
};

} // namespace ui
} // namespace rack
