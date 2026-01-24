#include "widget/PimplAdder.hpp"

namespace rack {
namespace widget {

// Define in order to allocated memory for the global map of internal structs
std::map<InternalStructsKey, void*> internalStructsMap_g;

}  // namespace widget
}  // namespace rack