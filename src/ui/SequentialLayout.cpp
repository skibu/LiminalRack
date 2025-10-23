#include <vector>

#include <ui/SequentialLayout.hpp>


namespace rack {
namespace ui {


SequentialLayout::SequentialLayout(Alignment alignment, bool if_two_rows_make_even) {
    this->alignment_ = alignment;
    this->if_two_rows_make_even_ = if_two_rows_make_even;
}


void SequentialLayout::step() {
    // Have superclass do its stuff
	Widget::step();

    // Determine how much space is available horizontally for the children
	available_width_ = X(getSize()) - 2 * X(margin_);

    // Go through all the children and add them to the current row. When a row
    // fills up create a new one.
    rows_.clear();
    Row row = Row();
    float current_row_width = 0.0f;

    for (widget::Widget* child : getChildren()) {
        // Skip invisible children
        if (!child->isVisible()) continue;

        // Determine minimum space needed for child
        float space_needed_for_child = X(child->getSize());
        if (row.size() > 0) {
            space_needed_for_child += X(min_spacing_);
        }

        // If row would not be too wide then add child to row
        if (current_row_width + space_needed_for_child <= available_width_) {
            // This child fits so add it to the row
            row.push_back(child);
            current_row_width += space_needed_for_child;
        } else {
            // This child would make the row too wide so store the curret row now that
            // it is complete, create another row and put the child into the new row.
            rows_.push_back(row);
            row = Row();
            row.push_back(child);
            current_row_width = X(child->getSize());
        }
    }

    // Add the last row to the list of rows
    rows_.push_back(row);

    // Handle special two row case
    makeSecondRowEven();

    // Update the layout of the children
    updateLayout();
}

float SequentialLayout::rowWidth(const Row& row) {
    float width = 0.0f;
    for (widget::Widget* child : row) {
        width += X(child->getSize());
    }
    return width;
}

float SequentialLayout::rowHeight(const Row& row) {
    float height = 0.0f;
    for (widget::Widget* child : row) {
        height = std::max(height, Y(child->getSize()));
    }
    return height;
}

float SequentialLayout::rowWidth(const Row& row, float spacing) {
    float width = 0.0f;
    for (widget::Widget* child : row) {
        width += X(child->getSize()) + spacing;
    }
    return width - spacing; // Remove last spacing
}

void SequentialLayout::makeSecondRowEven() {
    // If nothing to do then done
    if (!if_two_rows_make_even_ || rows_.size() != 2 || getChildren().size() < 2)
        return;

    Row& first_row = rows_[0];
    Row& second_row = rows_[1];
    while (rowWidth(first_row, X(min_spacing_)) >
           rowWidth(second_row, X(min_spacing_))) {
        // Move last child of first row to be first child of second row
        second_row.insert(second_row.begin(), first_row.back());
        first_row.pop_back();
    }
}

void SequentialLayout::updateLayout() {
    // Start vertical position at just below the top margin
    float y_position_of_child = Y(margin_);

    // For each row updates the layout of each child in that row
    for (const Row& row : rows_) {
        // Determine spacing to use between the children
        float spacing_to_use = X(min_spacing_);
        if (alignment_ == TAKE_ENTIRE_WIDTH) {
            // Spread children out to take up the entire width, but if margin would
            // be too big, greater than 10x min_spacing_, then cap it
            spacing_to_use =
                std::min((available_width_ - rowWidth(row)) / (row.size() - 1),
                         10 * X(min_spacing_));
        }

        // Determine left starting point for first child of row.
        float left_starting_point;
        switch (alignment_) {
            case LEFT_ALIGNMENT:
                // If left aligning then starting point is 0.0
                left_starting_point = 0.0f;
                break;
            case CENTER_ALIGNMENT:
                // Set left starting point to center the children
                left_starting_point = (available_width_ - rowWidth(row, spacing_to_use)) / 2;
                break;
            case RIGHT_ALIGNMENT:
                // Set left starting point to right align the children
                left_starting_point = (available_width_ - rowWidth(row, spacing_to_use));
                break;
            case TAKE_ENTIRE_WIDTH:
                // Same as CENTER_ALIGNMENT, except using a special spacing
                left_starting_point = (available_width_ - rowWidth(row, spacing_to_use)) / 2;
                break;
        }

        // Go through each child and set its position
        float left_pos = left_starting_point + X(margin_);
        for (widget::Widget* child : row) {
            // Set horizontal position of child
            auto child_pos = child->getPos();
            setX(child_pos, left_pos);
            child->setPos(child_pos);

            // Update left position for next child
            left_pos += X(child->getSize()) + spacing_to_use;

            // Set vertical position of child
            setY(child_pos, y_position_of_child);
            child->setPos(child_pos);
        }

        // Done with this row so update y_position_of_child for next row
        y_position_of_child += rowHeight(row) + Y(min_spacing_);

        // Update the height of the container
        auto size = getSize();
        setY(size, y_position_of_child - Y(min_spacing_));
        setSize(size);
    }
}

} // namespace ui
} // namespace rack
