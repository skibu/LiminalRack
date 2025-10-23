#pragma once
#include <vector>
#include <common.hpp>
#include <midi.hpp>

namespace rack {
namespace midiloopback {

// Forward declaration
class Device;

class Context {
   public:
    std::vector<Device*> devices;

    Context();
    ~Context();
};

PRIVATE void init();

}  // namespace midiloopback
}  // namespace rack
