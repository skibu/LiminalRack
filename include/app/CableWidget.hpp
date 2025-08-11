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
 * plug is drawn using the cables color. 
 */
struct PlugWidget : widget::Widget {
	// Forward declaration, needed because defined in cpp file
    struct PlugInternals;

    PlugInternals* plugInternals;

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


struct CableWidget : widget::Widget {
    // Forward declaration, needed because defined in cpp file
    struct CableInternal;
	
    CableInternal* cableInternals;

    /** Owned. */
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
	 * @brief Draws the cable and its shadow, though not the connectors, on the 
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
