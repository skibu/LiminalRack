#pragma once
#include <widget/OpaqueWidget.hpp>
#include <ui/common.hpp>
#include <context.hpp>


namespace rack {
namespace ui {

/**
 * @brief A text input field.
 *
 * Can be single-line or multi-line. Can also be a password field.
 */
class TextField : public widget::OpaqueWidget {
public:
    TextField();

    /** Sets the text content of the field. Also resets the cursor to the end of
     * the text and clears any selection.
     * NOTE: the original VCV Rack interface passes in text param by value instead
     * of the proper way of passing by const reference. Unfortunately some modules
     * like 4ms one uses this part of the old interface. Therefore should not change
     * the param to const reference or will get link errors.
     */
    void setText(std::string text);

    const std::string& getText() const {
        return text;
    }

    const char* getTextCStr() const {
        return text.c_str();
    }

    void setPassword(bool password) {
        this->password = password;
    }

    void setPlaceholder(const std::string& placeholder) {
        this->placeholder = placeholder;
    }

    void setNextField(Widget* nextField) {
        this->nextField = nextField;
    }

    int getCursor() const {
        return cursor;
    }

    int getSelection() const {
        return selection;
    }

    void setMultiline(bool multiline) {
        this->multiline = multiline;
    }

    void setFontSize(int fontSize) {
        this->fontSize = fontSize;
    }

    /** These action event functions are used externally so need to be public */
    void selectAll();

    /** Handles key presses and mouse clicks */
    void onSelectKey(const SelectKeyEvent& e) override;

    /** Inserts text at the cursor, replacing the selection if necessary */
    void insertText(std::string text);

    void copyClipboard();
    void cutClipboard();
    void pasteClipboard();
    const std::string getSelectedText() const;   

private:
    std::string text;
    std::string placeholder;
    bool multiline = false;

    /** Masks text with "*". */
    bool password = false;

    /** The index of the text cursor */
    int cursor = 0;

    /** The index of the other end of the selection.
    If nothing is selected, this is equal to `cursor`.
    */
    int selection = 0;

    /** Need to keep track of font size since it might be different from
     * the bnd default font size. This is important for calculating
     * text positions.
    */
    int fontSize = bndGetLabelFontSize();

    /** For Tab and Shift-Tab focusing.
    */
    Widget* prevField = NULL;
    Widget* nextField = NULL;

    void draw(const DrawArgs& args) override;
    void onDragHover(const DragHoverEvent& e) override;

    /** Called when user clicks on text area. Sets the cursor position. */
    void onButton(const ButtonEvent& e) override;

    /** Called when user types a regular character. */
    void onSelectText(const SelectTextEvent& e) override;

    /** Returns the text position corresponding to the given mouse position. 
     * Useful for setting the cursor position when the user clicks.
     */
    virtual int getTextPosition(math::Vec mousePos);

    void cursorToPrevWord();
    void cursorToNextWord();
	void cursorToLineStart();
	void cursorToLineEnd();

    /** Called when user right-clicks the text field. Pops up a context menu
     * that allows copy, cut, paste, and select all.
     */
    void createContextMenu();
};


class PasswordField : public TextField {
   public:
    PasswordField() {
        setPassword(true);
    }
};


} // namespace ui
} // namespace rack
