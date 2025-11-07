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
    CableInternal* cableInternals_;

    // Other members of CableWidget
    // The cable
    engine::Cable* cable_ = NULL;

    // The plugs on each end of the cable
    PlugWidget* inputPlug_;
    PlugWidget* outputPlug_;

    // The ports on module
    PortWidget* inputPort_ = NULL;
    PortWidget* outputPort_ = NULL;

    PortWidget* hoveredInputPort_ = NULL;
    PortWidget* hoveredOutputPort_ = NULL;

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
        return type == engine::Port::INPUT ? inputPlug_ : outputPlug_;
    }

	PortWidget*& getPort(engine::Port::Type type) {
		return type == engine::Port::INPUT ? inputPort_ : outputPort_;
	}

    PortWidget*& getHoveredPort(engine::Port::Type type) {
        return type == engine::Port::INPUT ? hoveredInputPort_
                                            : hoveredOutputPort_;
    }

    /** Gets the color of the cable */
    NVGcolor getColor() const {
        return color_;
    }

    /** Sets the color of the cable */
    void setColor(const NVGcolor& color) {
        color_ = color;
    }

   private:
    /** Color of the cable */
    NVGcolor color_;

    // Random value used to vary cable tension and perhaps make
    // cables look less uniform and thereby more natural. Multiplied
    // by random cable tension value and then added to the cable tension
    // for this cable. A value between -1.0 and 1.0
    float tensionRandomValue_ = 0.f;

    // Generates a random float value between -1.0 and 1.0
    static float generateTensionRandomValue();

    // Following positions are calculated in step() and used in draw().
    // They are in Rack coordinates since that is what the nvg drawing functions
    // expect.

    // Location of the output plug of cable
    math::Vec cableOutputPlugInRackCoords_;

    // Location of the output end of cable. This is slightly offset from the
    // plug position so that the cable appears to come out of the module plug
    // properly and not be drawn over the plug.
    math::Vec cableOutputEndInRackCoords_;

    // Location of the input plug of cable
    math::Vec cableInputPlugInRackCoords_;

    // Location of the input end of cable. This is slightly offset from the plug
    // position position so that the cable appears to come out of the module
    // plug properly and not be drawn over the plug.
    math::Vec cableInputEndInRackCoords_;

    // The slump vertex used to draw the cable bezier curve.
    // It is determined in step() and used in draw().
    math::Vec slumpVertexInRackCoords_;

    /** Calculates and returns the slump vertex (P1) position for the cable
     * bezier curve between the output (P0) and input (P2) ports. The slump
     * vertex position is based on the cable tension and the distance between
     * the ports. Coordinates are in Rack space.
     */
    math::Vec getSlumpVertexInRackCoords() const;

    /** Given input and output port locations and the rack bottom Y position,
     * returns what P1, the slump position, needs to be such that the drawing
     * of the cable would be exactly at the bottom of the rack.
     *
     * @return the x,y position of the slump point p1 such that the curve will
     * have its minimum at the bottom of the rack. In rack coordinates
     */
    math::Vec getSlumpVertexForCableAtScreenBottomInRackCoords() const;

    /** Returns the position of the input port of the module relative to Rack
     * coordinates, if cable has an input port. Otherwise returns position of
     * the input port being hovered over. Otherwise returns mouse position.
     * Positions are relative to the rack (not the screen).  */
    math::Vec getInputPortPosInRackCoords() const;

    /** Returns the position of the output port of the module relative to Rack
     * coordinates, if cable has an output port. Otherwise returns position of
     * the output port being hovered over. Otherwise returns mouse position.
     * Positions are relative to the rack (not the screen).  */
    math::Vec getOutputPortPosInRackCoords() const;

   public:  // public because following could be used by plugins
    void mergeJson(json_t* rootJ);
    void fromJson(json_t* rootJ);

    void step() override;
    void draw(const DrawArgs& args) override;

    /**
     * @brief Draws the cable and its shadow, though not the plugs, on the
     * specified layer.
     *
     * Key part is the opacity. If the cable is being manipulated then it is
     * drawn opaque, otherwise it is drawn somewhat translucent. The opacity
     * also affects the shadow.
     *
     * The nvg drawing functions assume that the coordinates passed in are in
     * Rack coordinates. Therefore all the positions need to be in Rack
     * coordinates before drawing.
     *
     * @param args
     * @param layer
     */
    void drawLayer(const DrawArgs& args, int layer) override;

    engine::Cable* releaseCable();

    void onAdd(const AddEvent& e) override;
    void onRemove(const RemoveEvent& e) override;

    static constexpr float CABLE_WIDTH = 6.0f;
    static constexpr float THICK_CABLE_WIDTH = 9.0f;
};

} // namespace app
} // namespace rack
