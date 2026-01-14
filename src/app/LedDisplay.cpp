#include <app/LedDisplay.hpp>
#include <asset.hpp>
#include <window/Window.hpp>
#include <context.hpp>


namespace rack {
namespace app {


void LedDisplay::draw(const DrawArgs& args) {
	math::Rect r = getBox().zeroPos();

	// Black background
	nvgBeginPath(args.vg);
	nvgRect(args.vg, RECT_ARGS(r));
	NVGcolor topColor = nvgRGB(0x22, 0x22, 0x22);
	NVGcolor bottomColor = nvgRGB(0x12, 0x12, 0x12);
	nvgFillPaint(args.vg, nvgLinearGradient(args.vg, 0.0, 0.0, 0.0, 25.0, topColor, bottomColor));
	// nvgFillColor(args.vg, bottomColor);
	nvgFill(args.vg);

	// Outer strokes
	nvgBeginPath(args.vg);
	nvgMoveTo(args.vg, 0.0, -0.5);
	nvgLineTo(args.vg, getWidth(), -0.5);
	nvgStrokeColor(args.vg, nvgRGBAf(0, 0, 0, 0.24));
	nvgStrokeWidth(args.vg, 1.0);
	nvgStroke(args.vg);

	nvgBeginPath(args.vg);
	nvgMoveTo(args.vg, 0.0, getHeight() + 0.5);
	nvgLineTo(args.vg, getWidth(), getHeight() + 0.5);
	nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, 0.25));
	nvgStrokeWidth(args.vg, 1.0);
	nvgStroke(args.vg);

	// Inner strokes
	nvgBeginPath(args.vg);
	nvgMoveTo(args.vg, 0.0, 2.5);
	nvgLineTo(args.vg, getWidth(), 2.5);
	nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, 0.20));
	nvgStrokeWidth(args.vg, 1.0);
	nvgStroke(args.vg);

	nvgBeginPath(args.vg);
	nvgMoveTo(args.vg, 0.0, getHeight() - 2.5);
	nvgLineTo(args.vg, getWidth(), getHeight() - 2.5);
	nvgStrokeColor(args.vg, nvgRGBAf(1, 1, 1, 0.20));
	nvgStrokeWidth(args.vg, 1.0);
	nvgStroke(args.vg);

	// Black border
	math::Rect rBorder = r.shrink(math::Vec(1, 1));
	nvgBeginPath(args.vg);
	nvgRect(args.vg, RECT_ARGS(rBorder));
	nvgStrokeColor(args.vg, bottomColor);
	nvgStrokeWidth(args.vg, 2.0);
	nvgStroke(args.vg);

	// Draw children inside box
	nvgScissor(args.vg, RECT_ARGS(args.clipBox));
	Widget::draw(args);
	nvgResetScissor(args.vg);
}


void LedDisplay::drawLayer(const DrawArgs& args, int layer) {
	// Draw children inside box
	nvgScissor(args.vg, RECT_ARGS(args.clipBox));
	Widget::drawLayer(args, layer);
	nvgResetScissor(args.vg);
}


LedDisplaySeparator::LedDisplaySeparator() {
	setSize(math::Vec());
}


void LedDisplaySeparator::draw(const DrawArgs& args) {
	nvgBeginPath(args.vg);
	nvgMoveTo(args.vg, 0, 0);
	nvgLineTo(args.vg, getWidth(), getHeight());
	nvgStrokeWidth(args.vg, 1.0);
	nvgStrokeColor(args.vg, nvgRGB(0x33, 0x33, 0x33));
	nvgStroke(args.vg);
}


LedDisplayChoice::LedDisplayChoice() {
	setSize(window::mm2px(math::Vec(0, 28.0 / 3)));
	fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
	textOffset = math::Vec(10, 18);
	color = nvgRGB(0xff, 0xd7, 0x14);
	bgColor = nvgRGBAf(0, 0, 0, 0);
}


void LedDisplayChoice::draw(const DrawArgs& args) {
	if (bgColor.a > 0.0) {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, getWidth(), getHeight());
		nvgFillColor(args.vg, bgColor);
		nvgFill(args.vg);
	}

	Widget::draw(args);
}


void LedDisplayChoice::drawLayer(const DrawArgs& args, int layer) {
	nvgScissor(args.vg, RECT_ARGS(args.clipBox));

	if (layer == 1) {
		std::shared_ptr<window::Font> font = getWindow()->loadFont(fontPath);
		if (font && font->handle >= 0) {
			nvgFillColor(args.vg, color);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextLetterSpacing(args.vg, 0.0);

			nvgFontSize(args.vg, 12);
			nvgText(args.vg, textOffset.getX(), textOffset.getY(), text.c_str(), NULL);
		}
	}

	Widget::drawLayer(args, layer);
	nvgResetScissor(args.vg);
}


void LedDisplayChoice::onButton(const ButtonEvent& e) {
	OpaqueWidget::onButton(e);

	if (e.action == GLFW_PRESS && (e.button == GLFW_MOUSE_BUTTON_LEFT || e.button == GLFW_MOUSE_BUTTON_RIGHT)) {
		ActionEvent eAction;
		onAction(eAction);
		e.consume(this);
	}
}


LedDisplayTextField::LedDisplayTextField() {
	fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
	textOffset = math::Vec(5, 5);
	color = nvgRGB(0xff, 0xd7, 0x14);
	bgColor = nvgRGB(0x00, 0x00, 0x00);
}


void LedDisplayTextField::draw(const DrawArgs& args) {
	Widget::draw(args);
}


void LedDisplayTextField::drawLayer(const DrawArgs& args, int layer) {
	nvgScissor(args.vg, RECT_ARGS(args.clipBox));

	if (layer == 1) {
		// Text
		std::shared_ptr<window::Font> font = getWindow()->loadFont(fontPath);
		if (font && font->handle >= 0) {
			bndSetFont(font->handle);

			NVGcolor highlightColor = color;
			highlightColor.a = 0.5;
            int cursor = getCursor();
            int selection = getSelection();
			int begin = std::min(cursor, selection);
			int end = (this == getEvent()->getSelectedWidget()) ? std::max(cursor, selection) : -1;
			bndIconLabelCaret(args.vg,
				textOffset.getX(), textOffset.getY(),
				getWidth() - 2 * textOffset.getX(), getHeight() - 2 * textOffset.getY(),
				-1, color, 12, getTextCStr(), highlightColor, begin, end);

			bndSetFont(getWindow()->uiFont_->handle);
		}
	}

	Widget::drawLayer(args, layer);
	nvgResetScissor(args.vg);
}

int LedDisplayTextField::getTextPosition(math::Vec mousePos) {
    std::shared_ptr<window::Font> font = getWindow()->loadFont(fontPath);
    if (!font || !font->handle) return 0;

    bndSetFont(font->handle);
    int textPos = bndIconLabelTextPosition(
        getWindow()->vg_, textOffset.getX(), textOffset.getY(),
        getWidth() - 2 * textOffset.getX(), getHeight() - 2 * textOffset.getY(),
        -1, 12, getTextCStr(), mousePos.getX(), mousePos.getY());
    bndSetFont(getWindow()->uiFont_->handle);
    return textPos;
}

} // namespace app
} // namespace rack
