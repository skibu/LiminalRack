#pragma once

#include <map>
#include <typeinfo>
#include <typeindex>

namespace rack {
namespace widget {

// Forward declaration
class Widget;

/** Defines the key to the map. Don't really want this to be in the header file
 * but need it here since PimplAdder is a templateclass.
 */
struct InternalStructsKey {
    InternalStructsKey(Widget* widgetPtr, const std::type_info& type)
        : widgetPtr_(widgetPtr), type_(std::type_index(type)) {}

    Widget* widgetPtr_;
    const std::type_index type_;

    // Comparison operator for use in std::map
    bool operator<(const InternalStructsKey& other) const {
        if (widgetPtr_ != other.widgetPtr_) {
            return widgetPtr_ < other.widgetPtr_;
        }
        return type_ < other.type_;
    }
};

// Declare the global map. Initialized in PimplAdder.cpp.
// The values need to be void* since there can be different types of 
// internal structs yet we want only a single map.
extern std::map<InternalStructsKey, void*> internalStructsMap_g;

/**
 * For adding internal structures to Widget classes as needed, without
 * breaking ABI compatibility. Adding a Internal pointer as a class member
 * would change the size of the class, creating potential issues for modules
 * compiled against the legacy Rack SDK. Instead, this PimplAdder class
 * provides a way to add internal data without affecting the class size. The
 * Internal data is instead held in a global map.
 * 
 * Example usage:
 * 
 * struct InternalsStruct {
 *   int someInternalData;
 *   float someOtherInternalData;
 * };
 * 
 * Widget:Widget() {
 *  // Make internal struct accessible via PimplAdder class
 *  PimplAdder<Widget, InternalsStruct>::create(this);
 * }
 * 
 * Widget::~Widget() {
 *   PimplAdder<Widget, InternalsStruct>::cleanup(this);
 * }
 * 
 * void Widget::setSomeData(int data) {
 *   InternalsStruct* internal = 
 *       PimplAdder<Widget, InternalsStruct>::get(this);
 *   internal->someInternalData = data;
 * }
 * 
 * int Widget::getSomeData() {
 *   InternalsStruct* internal = 
 *       PimplAdder<Widget, InternalsStruct>::get(this);
 *   return internal->someInternalData;
 * }
 */
template <typename TWidget, typename TInternalStruct>
class PimplAdder {
   public:
    /** Makes the PimplAdder accessible via PimplAdder class.
     * Allocates a new internal struct and stores it in the global map.
     */
    static void create(TWidget* widget) {
        // Allocate new internal struct
        void* internalStructPtr = new TInternalStruct();

        // Store in global map
        InternalStructsKey key(widget, typeid(TWidget));
        internalStructsMap_g[key] = internalStructPtr;
    }

    /** Returns pointer to internal struct of given type for widget, or nullptr
     * if none exists. */
    static TInternalStruct* get(TWidget* widget) {
        InternalStructsKey key(widget, typeid(TWidget));
        auto it = internalStructsMap_g.find(key);
        if (it != internalStructsMap_g.end()) {
            return (TInternalStruct*)it->second;
        }
        return nullptr;
    }

    /** Cleans up internal structures associated with a widget */
    static void cleanup(TWidget* widget) {
        void* internalStructPtr = get(widget);
        if (internalStructPtr) {
            delete (TInternalStruct*)internalStructPtr;
            InternalStructsKey key(widget, typeid(TWidget));
            internalStructsMap_g.erase(key);
        }
    }   
};


}  // namespace widget
}  // namespace rack