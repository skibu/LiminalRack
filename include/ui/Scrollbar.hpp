#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {

/** A Scrollbar. Uses blendish to actually draw the scrollbar. Parent must be a
 * ScrollWidget */
struct Scrollbar : widget::OpaqueWidget {
    // Note: Internal not actually used for Scrollbar. Just a placeholder.
   private:
    struct Internal;
    Internal* internal_;

   public:
    bool vertical = false;

    Scrollbar();
    ~Scrollbar();

    /** Draws the scrollbar */
	void draw(const DrawArgs& args) override;

	void onButton(const ButtonEvent& e) override;
	void onDragStart(const DragStartEvent& e) override;
	void onDragEnd(const DragEndEvent& e) override;
	void onDragMove(const DragMoveEvent& e) override;
};

DEPRECATED typedef Scrollbar ScrollBar;


} // namespace ui
} // namespace rack
