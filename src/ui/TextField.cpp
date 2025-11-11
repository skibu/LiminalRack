#include <ui/TextField.hpp>
#include <ui/MenuItem.hpp>
#include <helpers.hpp>
#include <context.hpp>

namespace rack {
namespace ui {


struct TextFieldCopyItem : ui::MenuItem {
	WeakPtr<TextField> textField;
	void onAction(const ActionEvent& e) override {
		if (!textField)
			return;
		textField->copyClipboard();
		getEvent()->setSelectedWidget(textField);
	}
};


struct TextFieldCutItem : ui::MenuItem {
	WeakPtr<TextField> textField;
	void onAction(const ActionEvent& e) override {
		if (!textField)
			return;
		textField->cutClipboard();
		getEvent()->setSelectedWidget(textField);
	}
};


struct TextFieldPasteItem : ui::MenuItem {
	WeakPtr<TextField> textField;
	void onAction(const ActionEvent& e) override {
		if (!textField)
			return;
		textField->pasteClipboard();
		getEvent()->setSelectedWidget(textField);
	}
};


struct TextFieldSelectAllItem : ui::MenuItem {
	WeakPtr<TextField> textField;
	void onAction(const ActionEvent& e) override {
		if (!textField)
			return;
		textField->selectAll();
		getEvent()->setSelectedWidget(textField);
	}
};


TextField::TextField() {
    setHeight(rack::settings::bndWidgetHeight);
}

void TextField::draw(const DrawArgs& args) {
    // Make sure single line text fields don't have text that
    // would be too long to fit in the box. If it does then trunacate the text.
    if (!multiline) {
        // Add exptra character to make sure we have room for cursor
        while (bndTextNeedsMultipleLines(args.vg, getWidth(), fontSize,
                                         (text+"X").c_str())) {
            // Truncate last character and try again
            text.resize(text.size() - 1);

            // Make sure cursor and selection are still valid
            if (cursor > (int)text.size()) {
                cursor = text.size();
            }
            if (selection > (int)text.size()) {
                selection = text.size();
            }
        }
    }

    nvgScissor(args.vg, RECT_ARGS(args.clipBox));

    BNDwidgetState state;
    if (this == getEvent()->selectedWidget)
        state = BND_ACTIVE;
    else if (this == getEvent()->hoveredWidget)
        state = BND_HOVER;
    else
        state = BND_DEFAULT;

    int begin = std::min(cursor, selection);
    int end = std::max(cursor, selection);

    std::string drawText;
    if (password) {
        drawText = std::string(string::UTF8Length(text), '*');
        begin = string::UTF8CodepointIndex(text, begin);
        end = string::UTF8CodepointIndex(text, end);
    } else {
        drawText = text;
    }

    // Draw the text field and associated text if any
    int font_size = settings::getLabelFontSize();
    bndTextField(args.vg, 0.0, 0.0, getWidth(), getHeight() - 4.0, BND_CORNER_NONE, state, -1,
                    drawText.c_str(), begin, end);

    // Draw dimmed placeholder text if no text entered
    if (text.empty()) {
        auto theme = bndGetTheme()->textFieldTheme;
        NVGcolor dim_text_color =
            color::lerp(theme.innerColor, theme.textColor, 0.4);
        float y_offset = -BND_TEXT_PAD_DOWN + (getHeight() - font_size) / 2.0f;
        bndIconLabelCaret(args.vg, 0.0, y_offset, getWidth(), getHeight(),
                          -1 /* no icon */, dim_text_color,
                          -1 /* use already set font size */,
                          placeholder.c_str(), dim_text_color, 0, -1);
    }

    nvgResetScissor(args.vg);
}

void TextField::onDragHover(const DragHoverEvent& e) {
	OpaqueWidget::onDragHover(e);

	if (e.origin == this) {
		int pos = getTextPosition(e.pos);
		cursor = pos;
	}
}

void TextField::onButton(const ButtonEvent& e) {
	OpaqueWidget::onButton(e);

	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
		cursor = selection = getTextPosition(e.pos);
	}

	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
		createContextMenu();
		e.consume(this);
	}
}

void TextField::onSelectText(const SelectTextEvent& e) {
	std::u32string s32(1, char32_t(e.codepoint));
	std::string s8 = string::UTF32toUTF8(s32);
	insertText(s8);
	e.consume(this);
}

void TextField::onSelectKey(const SelectKeyEvent& e) {
#if defined ARCH_MAC
	#define TEXTFIELD_MOD_CTRL RACK_MOD_ALT
#else
	#define TEXTFIELD_MOD_CTRL RACK_MOD_CTRL
#endif

	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		// Backspace
		if (e.isKeyCommand(GLFW_KEY_BACKSPACE)) {
			if (cursor == selection) {
				cursor = string::UTF8PrevCodepoint(text, cursor);
			}
			insertText("");
			e.consume(this);
		}
		// Ctrl+Backspace
		if (e.isKeyCommand(GLFW_KEY_BACKSPACE, TEXTFIELD_MOD_CTRL)) {
			if (cursor == selection) {
				cursorToPrevWord();
			}
			insertText("");
			e.consume(this);
		}
		// Delete
		if (e.isKeyCommand(GLFW_KEY_DELETE)) {
			if (cursor == selection) {
				cursor = string::UTF8NextCodepoint(text, cursor);
			}
			insertText("");
			e.consume(this);
		}
		// Ctrl+Delete
		if (e.isKeyCommand(GLFW_KEY_DELETE, TEXTFIELD_MOD_CTRL)) {
			if (cursor == selection) {
				cursorToNextWord();
			}
			insertText("");
			e.consume(this);
		}
		// Left
		if (e.isKeyCommand(GLFW_KEY_LEFT)) {
			cursor = string::UTF8PrevCodepoint(text, cursor);
			selection = cursor;
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_LEFT, TEXTFIELD_MOD_CTRL)) {
			cursorToPrevWord();
			selection = cursor;
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_LEFT, GLFW_MOD_SHIFT)) {
			cursor = string::UTF8PrevCodepoint(text, cursor);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_LEFT, TEXTFIELD_MOD_CTRL | GLFW_MOD_SHIFT)) {
			cursorToPrevWord();
			e.consume(this);
		}
		// Right
		if (e.isKeyCommand(GLFW_KEY_RIGHT)) {
			cursor = string::UTF8NextCodepoint(text, cursor);
			selection = cursor;
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_RIGHT, TEXTFIELD_MOD_CTRL)) {
			cursorToNextWord();
			selection = cursor;
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_RIGHT, GLFW_MOD_SHIFT)) {
			cursor = string::UTF8NextCodepoint(text, cursor);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_RIGHT, TEXTFIELD_MOD_CTRL | GLFW_MOD_SHIFT)) {
			cursorToNextWord();
			e.consume(this);
		}
		// Up (placeholder)
		if (e.isKeyCommand(GLFW_KEY_UP)) {
			e.consume(this);
		}
		// Down (placeholder)
		if (e.isKeyCommand(GLFW_KEY_DOWN)) {
			e.consume(this);
		}
		// Home
		if (e.isKeyCommand(GLFW_KEY_HOME)
#if defined ARCH_MAC
			|| e.isKeyCommand(GLFW_KEY_LEFT, RACK_MOD_CTRL)
#endif
		) {
			selection = cursor = 0;
			e.consume(this);
		}
		// Shift+Home
		if (e.isKeyCommand(GLFW_KEY_HOME, GLFW_MOD_SHIFT)
#if defined ARCH_MAC
			|| e.isKeyCommand(GLFW_KEY_LEFT, RACK_MOD_CTRL | GLFW_MOD_SHIFT)
#endif
		) {
			cursor = 0;
			e.consume(this);
		}
		// End
		if (e.isKeyCommand(GLFW_KEY_END)
#if defined ARCH_MAC
			|| e.isKeyCommand(GLFW_KEY_RIGHT, RACK_MOD_CTRL)
#endif
		) {
			selection = cursor = text.size();
			e.consume(this);
		}
		// Shift+End
		if (e.isKeyCommand(GLFW_KEY_END, GLFW_MOD_SHIFT)
#if defined ARCH_MAC
			|| e.isKeyCommand(GLFW_KEY_RIGHT, RACK_MOD_CTRL | GLFW_MOD_SHIFT)
#endif
		) {
			cursor = text.size();
			e.consume(this);
		}
		// Ctrl+V
		if (e.isKeyCommand(GLFW_KEY_V, RACK_MOD_CTRL)) {
			pasteClipboard();
			e.consume(this);
		}
		// Ctrl+X
		if (e.isKeyCommand(GLFW_KEY_X, RACK_MOD_CTRL)) {
			cutClipboard();
			e.consume(this);
		}
		// Ctrl+C
		if (e.isKeyCommand(GLFW_KEY_C, RACK_MOD_CTRL)) {
			copyClipboard();
			e.consume(this);
		}
		// Ctrl+A
		if (e.isKeyCommand(GLFW_KEY_A, RACK_MOD_CTRL)) {
			selectAll();
			e.consume(this);
		}
		// Enter
		if (e.isKeyCommand(GLFW_KEY_ENTER) || e.isKeyCommand(GLFW_KEY_KP_ENTER)) {
			if (multiline) {
				insertText("\n");
			}
			else {
				ActionEvent eAction;
				onAction(eAction);
			}
			e.consume(this);
		}
		// Tab
		if (e.isKeyCommand(GLFW_KEY_TAB)) {
			if (nextField)
				getEvent()->setSelectedWidget(nextField);
			e.consume(this);
		}
		// Shift+Tab
		if (e.isKeyCommand(GLFW_KEY_TAB, GLFW_MOD_SHIFT)) {
			if (prevField)
				getEvent()->setSelectedWidget(prevField);
			e.consume(this);
		}
		// Consume all printable keys unless Ctrl is held
		if (GLFW_KEY_SPACE <= e.key && e.key < 128 && (e.mods & RACK_MOD_CTRL) == 0) {
			e.consume(this);
		}

		assert(0 <= cursor);
		assert(cursor <= (int) text.size());
		assert(0 <= selection);
		assert(selection <= (int) text.size());
	}
}

int TextField::getTextPosition(math::Vec mousePos) {
    return bndTextFieldTextPosition(getWindow()->vg_, 0.0, 0.0, getWidth(),
                                    getHeight(), -1, fontSize, text.c_str(),
                                    mousePos.getX(), mousePos.getY());
}

void TextField::setText(std::string text) {
	if (this->text != text) {
		this->text = text;
		// ChangeEvent
		ChangeEvent eChange;
		onChange(eChange);
	}
	selection = cursor = text.size();
}

void TextField::selectAll() {
	cursor = text.size();
	selection = 0;
}

const std::string TextField::getSelectedText() const {
	int begin = std::min(cursor, selection);
	int len = std::abs(selection - cursor);
	return text.substr(begin, len);
}

void TextField::insertText(std::string text) {
	// Rack uses UNIX newlines so remove all CR characters
	text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());

	bool changed = false;
	if (cursor != selection) {
		// Delete selected text
		int begin = std::min(cursor, selection);
		int len = std::abs(selection - cursor);
		this->text.erase(begin, len);
		cursor = selection = begin;
		changed = true;
	}
	if (!text.empty()) {
		this->text.insert(cursor, text);
		cursor += text.size();
		selection = cursor;
		changed = true;
	}
	if (changed) {
		ChangeEvent eChange;
		onChange(eChange);
	}
}

void TextField::copyClipboard() {
    if (cursor == selection) return;
    glfwSetClipboardString(getWindow()->getGLFWwindow(),
                           getSelectedText().c_str());
}

void TextField::cutClipboard() {
	copyClipboard();
	insertText("");
}

void TextField::pasteClipboard() {
    const char* newText = glfwGetClipboardString(getWindow()->getGLFWwindow());
    if (!newText) return;
    insertText(newText);
}

void TextField::cursorToPrevWord() {
	if (password) {
		cursor = 0;
		return;
	}
	// This works for valid UTF-8 text
	size_t pos = text.rfind(' ', std::max(cursor - 2, 0));
	if (pos == std::string::npos)
		cursor = 0;
	else
		cursor = std::min((int) pos + 1, (int) text.size());
}

void TextField::cursorToNextWord() {
	if (password) {
		cursor = text.size();
		return;
	}
	// This works for valid UTF-8 text
	size_t pos = text.find(' ', std::min(cursor + 1, (int) text.size()));
	if (pos == std::string::npos)
		pos = text.size();
	cursor = pos;
}

void TextField::createContextMenu() {
	ui::Menu* menu = createMenu();

	TextFieldCutItem* cutItem = new TextFieldCutItem;
	cutItem->setText(string::translate("TextField.cut"));
	cutItem->setRightText(widget::getKeyCommandName(GLFW_KEY_X, RACK_MOD_CTRL));
	cutItem->textField = this;
	menu->addChild(cutItem);

	TextFieldCopyItem* copyItem = new TextFieldCopyItem;
	copyItem->setText(string::translate("TextField.copy"));
	copyItem->setRightText(widget::getKeyCommandName(GLFW_KEY_C, RACK_MOD_CTRL));
	copyItem->textField = this;
	menu->addChild(copyItem);

	TextFieldPasteItem* pasteItem = new TextFieldPasteItem;
	pasteItem->setText(string::translate("TextField.paste"));
	pasteItem->setRightText(widget::getKeyCommandName(GLFW_KEY_V, RACK_MOD_CTRL));
	pasteItem->textField = this;
	menu->addChild(pasteItem);

	TextFieldSelectAllItem* selectAllItem = new TextFieldSelectAllItem;
	selectAllItem->setText(string::translate("TextField.selectAll"));
	selectAllItem->setRightText(widget::getKeyCommandName(GLFW_KEY_A, RACK_MOD_CTRL));
	selectAllItem->textField = this;
	menu->addChild(selectAllItem);
}


} // namespace ui
} // namespace rack
