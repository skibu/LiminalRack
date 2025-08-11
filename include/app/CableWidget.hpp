#pragma once
#include <map>
#include <app/common.hpp>
#include <widget/Widget.hpp>
#include <app/PortWidget.hpp>
#include <engine/Cable.hpp>


namespace rack {
namespace app {

// Forward declaration, needed because used in PlugWidget but
// defined afterwards in this file
struct CableWidget;

/** 
 * The PlugWidget defines one of the plugs of a cable. The
 * plug is drawn using the color of the cable. It should be noted
 * that the Port is separate and is the jack that the plug is plugged into.
 */
struct PlugWidget : widget::Widget {
	// Forward declaration, needed because defined in cpp file
    struct PlugInternals;

	// The members of PlugWidget
    PlugInternals* plugInternals;

    /**
     * @brief Construct a new Plug Widget so that it can be drawn
     *
     * @details Drawing is done by first drawing PlugInput or PlugOutput, and
     * then drawing PlugPort on top of it. Goal is to have output port look like
     * an arrow pointing away from the jack, and the input port look like an
     * arrow pointing towards the jack. This should help user better understand
     * flow of signals in the patch.
     *
     * @param cableWidget
     * @param type INPUT or OUTPUT
     */
    PlugWidget(const CableWidget* cableWidget, engine::Port::Type type);
    ~PlugWidget();

    void step() override;

	// The color of the plug, set by CableWidget.
    PRIVATE void setColor(NVGcolor color);

	// The angle that the SVG of the plug should be rotated
    PRIVATE void setAngle(float angle);

	// Specifies if the plug is on top of possibly multiple plugs
    PRIVATE void setTop(bool top);

	// Returns the parent cable widget
    CableWidget* getCable();

	// Returns the type of the plug, either INPUT or OUTPUT
    engine::Port::Type getType();
};


/** 
 * The CableWidget defines a cable that connects two ports together.
 * It is drawn as a curved line with along with a shadow. The main
 * cable has a plug on each end, one an input and one an output. Different
 * SVG drawings are used to draw the input and output plugs so that the
 * user can easily see the direction of signal flow.
 * an arrow at the end of the output plug.
 * 
 * The CableWidget also keeps track of the ports on the modules that it is
 * connected to, but is not responsible for drawing the ports. The ports are
 * drawn by the PortWidget.
 */
struct CableWidget : widget::Widget {
    // Forward declaration, needed because defined in cpp file
    struct CableInternal;

	// The internal members of PlugWidget
    CableInternal* cableInternals;

    // Other members of CableWidget
    engine::Cable* cable = NULL;
    NVGcolor color;
    PlugWidget* inputPlug;
    PlugWidget* outputPlug;

    PortWidget* inputPort = NULL;
    PortWidget* outputPort = NULL;
    PortWidget* hoveredInputPort = NULL;
    PortWidget* hoveredOutputPort = NULL;

    CableWidget();
    ~CableWidget();

    /** Returns whether cable is connected to 2 ports. */
    bool isComplete();

    /** Based on the input/output ports, re-creates the cable and removes/adds it to the Engine. */
    void updateCable();

    /** From a cable, sets the input/output ports.
    Cable must already be added to the Engine.
    Adopts ownership.
    */
    void setCable(engine::Cable* cable);

    engine::Cable* getCable();

    PlugWidget*& getPlug(engine::Port::Type type) {
        return type == engine::Port::INPUT ? inputPlug : outputPlug;
    }

	PortWidget*& getPort(engine::Port::Type type) {
		return type == engine::Port::INPUT ? inputPort : outputPort;
	}

	PortWidget*& getHoveredPort(engine::Port::Type type) {
		return type == engine::Port::INPUT ? hoveredInputPort : hoveredOutputPort;
	}

	math::Vec getInputPos();
	math::Vec getOutputPos();
	void mergeJson(json_t* rootJ);
	void fromJson(json_t* rootJ);
	void step() override;
	void draw(const DrawArgs& args) override;

	/**
	 * @brief Draws the cable and its shadow, though not the plugs, on the 
	 * specified layer. 
	 * 
	 * Key part is the opacity. If the cable is being manipulated
	 * then it is drawn opaque, otherwise it is drawn somewhat translucent. The
	 * opacity also affects the shadow.
	 * 
	 * @param args 
	 * @param layer 
	 */
	void drawLayer(const DrawArgs& args, int layer) override;

	engine::Cable* releaseCable();

	void onAdd(const AddEvent& e) override;
	void onRemove(const RemoveEvent& e) override;
};


} // namespace app
} // namespace rack
