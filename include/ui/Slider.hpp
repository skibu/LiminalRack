#pragma once
#include <widget/OpaqueWidget.hpp>
#include <Quantity.hpp>
#include <ui/common.hpp>
#include <context.hpp>


namespace rack {
namespace ui {

class Slider : public widget::OpaqueWidget {
   public:
    /** The proper constructor, which makes sure that quantity is set */
    Slider(Quantity* quantity) : Slider() {
        setQuantity(quantity);
    }

    /** The default constructor is needed since it is part of the API and used
     * by third-party modules. */
    Slider();

    void setQuantity(Quantity* quantity) {
        this->quantity = quantity;
    }

    /** Needs to be public because part of API and third-party modules use it */
    Quantity* quantity;

   protected:
    void draw(const DrawArgs& args) override;
    void onDragStart(const DragStartEvent& e) override;
    void onDragMove(const DragMoveEvent& e) override;
    void onDragEnd(const DragEndEvent& e) override;
    void onDoubleClick(const DoubleClickEvent& e) override;
};

} // namespace ui
} // namespace rack
