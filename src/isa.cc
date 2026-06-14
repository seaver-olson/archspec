#include "aview.hh"

namespace archspec {

IsaFeatures Collector::collect_isa() const {
  return {};
}

IsaFeatures collect_isa() { return Collector{}.collect_isa();}

}