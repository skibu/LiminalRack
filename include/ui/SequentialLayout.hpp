#pragma once
#include <widget/Widget.hpp>
#include <ui/common.hpp>


namespace rack {
namespace ui {


/** A container of children widgets. Positions children in a row/column based on their widths/heights */
class SequentialLayout : public widget::Widget {
   public:
   
	enum Orientation {
        // The default layout of horizontal rows
		HORIZONTAL_ORIENTATION,
        // Layout of vertical columns
		VERTICAL_ORIENTATION,
	};

	enum Alignment {
        // Children packed together on left
		LEFT_ALIGNMENT,
        // Children packed together in center
		CENTER_ALIGNMENT,
        // Children packed together on right
		RIGHT_ALIGNMENT,
        // Adjusts spacing so that children take up entire available width
        TAKE_ENTIRE_WIDTH, 

		TOP_ALIGNMENT = LEFT_ALIGNMENT,
		MIDDLE_ALIGNMENT = CENTER_ALIGNMENT,
		BOTTOM_ALIGNMENT = RIGHT_ALIGNMENT,
	};

    SequentialLayout(Alignment alignment = LEFT_ALIGNMENT, bool if_two_rows_make_even = false);

    void setMinSpacing(const math::Vec& spacing) {
        min_spacing_ = spacing;
    }

    /** Sets the margin around the entire layout */
    void setMargin(const math::Vec& margin) {
        margin_ = margin;
    }

    void setAlignment(Alignment alignment) {
        alignment_ = alignment;
    }

    void setOrientation(Orientation orientation) {
        orientation_ = orientation;
    }

    void setWrap(bool wrap) {
        wrap_ = wrap;
    }

    /** Does the actual layout of the children */
	void step() override;

   private:
   /** Orientation of the layout, whether horizontal or vertical */
   	Orientation orientation_ = HORIZONTAL_ORIENTATION;

    /** Alignment of the children within the layout */
	Alignment alignment_ = LEFT_ALIGNMENT;

    /** When there is no max size for row of children, like for a tooltip */
    bool wrap_ = true;

    /** If enabled and there are two rows of children then the children will be
     * redistributed so that the rows are roughly equal in length. This is so
     * that you don't get just a single straggler on the second row, which can
     * look odd.
     */
    bool if_two_rows_make_even_ = false;

    /** Space between box bounds. */
    math::Vec margin_;

    /** Minimum space between adjacent children, and adjacent lines if wrapped.
     */
    math::Vec min_spacing_;

    /** Space available for the row of children */
    float available_width_ = 0.0f;

    // Simplify defining Rows
    using Row = std::vector<widget::Widget*>;

    // Contains the rows of children
    std::vector<Row> rows_ = std::vector<Row>();

    /** Helper function to access the correct axis based on orientation */
    float X(math::Vec v) {
        return orientation_ == HORIZONTAL_ORIENTATION ? (v).getX() : (v).getY();
    }

    /** Helper function to access the correct axis based on orientation */
    float Y(math::Vec v) {
        return orientation_ == HORIZONTAL_ORIENTATION ? (v).getY() : (v).getX();
    }

    /** Updates the X value of vec, depending on the orientation */
    void setX(math::Vec& vec, float value) {
        if (orientation_ == HORIZONTAL_ORIENTATION)
            vec.setX(value);
        else
            vec.setY(value);
    }

    /** Updates the Y value of vec, depending on the orientation */
    void setY(math::Vec& vec, float value) {
        if (orientation_ == HORIZONTAL_ORIENTATION)
            vec.setY(value);
        else
            vec.setX(value);
    }

    /** Returns minimum width needed for the children in the row, without spacing between the
     * children */
    float rowWidth(const Row& row);

    /** Returns minimum width needed for the children in the row, including the
     * specified spacing between them */
    float rowWidth(const Row& row, float spacing);

    /** Returns the height of the row, which is the maximum height of its children */
    float rowHeight(const Row& row);

    /** If enabled and there are two rows of children then the children will be
     * redistributed so that the rows are roughly equal in length. This is so
     * that you don't get just a single straggler on the second row, which can
     * look odd.
     */
    void makeSecondRowEven();

    /** To be called when children have been put into the rows such that the
     * allowed width of each row will not be exceeded. Goes through each row of
     * matrix and updates the layout of each child in that row */
    void updateLayout();
};


} // namespace ui
} // namespace rack
