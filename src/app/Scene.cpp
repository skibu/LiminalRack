#include <thread>

#include <osdialog.h>

#include <app/Scene.hpp>
#include <app/Browser.hpp>
#include <app/TipWindow.hpp>
#include <app/MenuBar.hpp>
#include <context.hpp>
#include <system.hpp>
#include <network.hpp>
#include <history.hpp>
#include <settings.hpp>
#include <patch.hpp>
#include <asset.hpp>
#include <app/Scene.hpp>


namespace rack {
namespace app {

/** ResizeHandle is a triangular handle placed in lower right side
 * of window for changing window size. But it isn't currently being used.
 */
class ResizeHandle : public widget::OpaqueWidget {
    public:
	ResizeHandle() {
		setSize(math::Vec(15, 15));

		// Currently not used so hide it
		hide();
	}

   private:
    math::Vec size;

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		math::Vec s = getSize();
		nvgMoveTo(args.vg, s.getX(), s.getY());
		nvgLineTo(args.vg, 0, s.getY());
		nvgLineTo(args.vg, s.getX(), 0);
		nvgClosePath(args.vg);
		// To show that resize handle not actually used color is set to green
		nvgFillColor(args.vg, nvgRGBAf(1, 1, 1, 0.15));
		nvgFill(args.vg);
	}

    void onDragStart(const DragStartEvent& e) override {
        size = getWindow()->getSize();
    }

    void onDragMove(const DragMoveEvent& e) override {
        size = size.plus(e.mouseDelta);
        getWindow()->setSize(size.round());
    }
};

struct Scene::Internal {
    // Note: not currently used
	ResizeHandle* resizeHandle;

	double lastAutosaveTime = 0.0;

	bool heldArrowKeys[4] = {};
};


Scene::Scene() {
    DEBUG("Constructing the Scene Widget...");

	internal_ = new Internal;

    // Create the scrolled rack area
	rackScroll = new RackScrollWidget;
	addChild(rackScroll);
	rack = rackScroll->rackWidget;

    // Create menu bar
	menuBar = createMenuBar();
	addChild(menuBar);

    // Create module browser window but keep it hidden for now
	browser = browserCreate();
	addChild(browser);

    // Create tip window if enabled in settings
	if (settings::showTipsOnLaunch) {
		addChild(tipWindowCreate());
	}

    // Note: resizeHandle is created but hidden and never unhidden. Therefore
    // it is not actually needed.
	internal_->resizeHandle = new ResizeHandle;
	addChild(internal_->resizeHandle);
}


Scene::~Scene() {
	delete internal_;
}

math::Vec Scene::getMousePos() {
	return mousePos;
}

widget::Widget* Scene::getMenuBar() {
    return menuBar;
}

RackWidget* Scene::getRack() {
    return rack;
}

RackScrollWidget* Scene::getRackScroll() {
    return rackScroll;
}

widget::Widget* Scene::getBrowser() {
    return browser;
}

void Scene::step() {
	if (getWindow()->isFullScreen()) {
		// Expand RackScrollWidget to cover entire screen if fullscreen
		rackScroll->setPos(math::Vec(rackScroll->getPos().getX(), 0));
	} else {
		// Always show MenuBar if not fullscreen
		menuBar->show();
		rackScroll->setPos(math::Vec(rackScroll->getPos().getX(), menuBar->getSize().getY()));
	}

	internal_->resizeHandle->setPos(getSize().minus(internal_->resizeHandle->getSize()));

	// Resize owned descendants
	menuBar->setSize(math::Vec(getSize().getX(), menuBar->getSize().getY()));
	rackScroll->setSize(getSize().minus(rackScroll->getPos()));
    rackScroll->setSize(getSize().minus(rackScroll->getPos()));

    // Autosave periodically
    if (settings::autosaveInterval > 0.0) {
        double time = system::getTime();
        if (time - internal_->lastAutosaveTime >= settings::autosaveInterval) {
            internal_->lastAutosaveTime = time;
            getPatch()->saveAutosave();
            settings::save();
        }
    }

	// Scroll RackScrollWidget with arrow keys
	math::Vec arrowDelta(0,0);
	if (internal_->heldArrowKeys[0]) {
		arrowDelta.setX(arrowDelta.getX() - 1);
	}
	if (internal_->heldArrowKeys[1]) {
		arrowDelta.setX(arrowDelta.getX() + 1);
	}
	if (internal_->heldArrowKeys[2]) {
		arrowDelta.setY(arrowDelta.getY() - 1);
	}
	if (internal_->heldArrowKeys[3]) {
		arrowDelta.setY(arrowDelta.getY() + 1);
	}

    if (!arrowDelta.isZero()) {
        int mods = getWindow()->getMods();
        float arrowSpeed = 32.f;
        if ((mods & RACK_MOD_MASK) == RACK_MOD_CTRL) arrowSpeed /= 4.f;
        if ((mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) arrowSpeed *= 4.f;
        if ((mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT))
            arrowSpeed /= 16.f;

        rackScroll->offset += arrowDelta * arrowSpeed;
    }

    Widget::step();
}

void Scene::draw(const DrawArgs& args) {
	Widget::draw(args);
}


void Scene::onHover(const HoverEvent& e) {
	mousePos = e.pos;
	if (mousePos.getY() < menuBar->getHeight()) {
		menuBar->show();
	}
	OpaqueWidget::onHover(e);
}


void Scene::onDragHover(const DragHoverEvent& e) {
	mousePos = e.pos;
	OpaqueWidget::onDragHover(e);
}


void Scene::onHoverKey(const HoverKeyEvent& e) {
	// Key commands that override children
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		// DEBUG("key %d '%c' scancode %d keyName '%s' mods %02x", e.key, e.key, e.scancode, e.keyName.c_str(), e.mods);
		if (e.isKeyCommand(GLFW_KEY_N, RACK_MOD_CTRL)) {
			getPatch()->loadTemplateDialog();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_Q, RACK_MOD_CTRL)) {
			getWindow()->close();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_O, RACK_MOD_CTRL)) {
			getPatch()->loadDialog();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_O, RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			getPatch()->revertDialog();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_S, RACK_MOD_CTRL)) {
			getPatch()->saveDialog();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_S, RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			getPatch()->saveAsDialog();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_Z, RACK_MOD_CTRL)) {
			getHistory()->undo();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_Z, RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			getHistory()->redo();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_MINUS, RACK_MOD_CTRL) || e.isKeyCommand(GLFW_KEY_KP_SUBTRACT, RACK_MOD_CTRL)) {
			float zoom = std::log2(getScene()->getRackScroll()->getZoom());
			zoom *= 2;
			zoom = std::ceil(zoom - 0.01f) - 1;
			zoom /= 2;
			getScene()->rackScroll->setZoom(std::pow(2.f, zoom));
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_EQUAL, RACK_MOD_CTRL)
			// The user might hold shift to access the + character
			|| e.isKeyCommand(GLFW_KEY_EQUAL, RACK_MOD_CTRL | GLFW_MOD_SHIFT)
			// Numpad + key
			|| e.isKeyCommand(GLFW_KEY_KP_ADD, RACK_MOD_CTRL)
			// Some layouts (e.g. QWERTZ) have a + key, but GLFW doesn't have a macro for it
			|| e.isKeyCommand('+', RACK_MOD_CTRL)) {
			float zoom = std::log2(getScene()->rackScroll->getZoom());
			zoom *= 2;
			zoom = std::floor(zoom + 0.01f) + 1;
			zoom /= 2;
			getScene()->rackScroll->setZoom(std::pow(2.f, zoom));
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_0, RACK_MOD_CTRL) || e.isKeyCommand(GLFW_KEY_KP_0, RACK_MOD_CTRL)) {
			getScene()->rackScroll->setZoom(1.f);
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_F1)) {
			system::openBrowser("https://vcvrack.com/manual/");
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_F3)) {
			settings::cpuMeter ^= true;
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_F4)) {
			getScene()->rackScroll->zoomToModules();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_F11)) {
			getWindow()->setFullScreen(!getWindow()->isFullScreen());
			// The MenuBar will be hidden when the mouse moves over the RackScrollWidget.
			// menuBar->hide();
			e.consume(this);
		}

		// Module selections
		if (e.isKeyCommand(GLFW_KEY_A, RACK_MOD_CTRL)) {
			rack->selectAll();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_A, RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			rack->deselectAll();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_C, RACK_MOD_CTRL)) {
			if (rack->hasSelection()) {
				rack->copyClipboardSelection();
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_I, RACK_MOD_CTRL)) {
			if (rack->hasSelection()) {
				rack->resetSelectionAction();
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_R, RACK_MOD_CTRL)) {
			if (rack->hasSelection()) {
				rack->randomizeSelectionAction();
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_U, RACK_MOD_CTRL)) {
			if (rack->hasSelection()) {
				rack->disconnectSelectionAction();
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_E, RACK_MOD_CTRL)) {
			if (rack->hasSelection()) {
				rack->bypassSelectionAction(!rack->isSelectionBypassed());
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_D, RACK_MOD_CTRL)) {
			if (rack->hasSelection()) {
				rack->cloneSelectionAction(false);
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_D, RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			if (rack->hasSelection()) {
				rack->cloneSelectionAction(true);
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_DELETE) || e.isKeyCommand(GLFW_KEY_BACKSPACE)) {
			if (rack->hasSelection()) {
				rack->deleteSelectionAction();
				e.consume(this);
			}
		}
	}

	// Scroll RackScrollWidget with arrow keys
	if (e.action == GLFW_PRESS || e.action == GLFW_RELEASE) {
		if (e.key == GLFW_KEY_LEFT) {
			internal_->heldArrowKeys[0] = (e.action == GLFW_PRESS);
			e.consume(this);
		}
		if (e.key == GLFW_KEY_RIGHT) {
			internal_->heldArrowKeys[1] = (e.action == GLFW_PRESS);
			e.consume(this);
		}
		if (e.key == GLFW_KEY_UP) {
			internal_->heldArrowKeys[2] = (e.action == GLFW_PRESS);
			e.consume(this);
		}
		if (e.key == GLFW_KEY_DOWN) {
			internal_->heldArrowKeys[3] = (e.action == GLFW_PRESS);
			e.consume(this);
		}
	}

	if (e.isConsumed())
		return;
	OpaqueWidget::onHoverKey(e);
	if (e.isConsumed())
		return;

	// Key commands that can be overridden by children
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		// Alternative key command for exiting fullscreen, since F11 doesn't work reliably on Mac due to "Show desktop" OS binding.
		if (e.isKeyCommand(GLFW_KEY_ESCAPE, 0)) {
			if (getWindow()->isFullScreen()) {
				getWindow()->setFullScreen(false);
				e.consume(this);
			}
		}
		if (e.isKeyCommand(GLFW_KEY_V, RACK_MOD_CTRL)) {
			rack->pasteClipboardAction();
			e.consume(this);
		}
		if (e.isKeyCommand(GLFW_KEY_ENTER) || e.isKeyCommand(GLFW_KEY_KP_ENTER)) {
			browser->show();
			e.consume(this);
		}
	}
}


void Scene::onPathDrop(const PathDropEvent& e) {
	if (e.paths.size() >= 1) {
		const std::string& path = e.paths[0];
		std::string extension = system::getExtension(path);

		if (extension == ".vcv") {
			getPatch()->loadPathDialog(path);
			e.consume(this);
			return;
		}
		if (extension == ".vcvs") {
			getRack()->loadSelection(path);
			e.consume(this);
			return;
		}
	}

	OpaqueWidget::onPathDrop(e);
}


} // namespace app
} // namespace rack
