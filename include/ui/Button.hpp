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
    Button(const std::string& text) : Button() {
        setText(text);
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
    std::string text_;

    /** Not owned. Tracks the pressed state of the button.*/
    Quantity* quantity_ = NULL;

   private:
    void draw(const DrawArgs& args) override;
    void onDragStart(const DragStartEvent& e) override;
    void onDragEnd(const DragEndEvent& e) override;
    void onDragDrop(const DragDropEvent& e) override;
};

} // namespace ui
} // namespace rack
