#include "aview.hh"

namespace archspec {

ThermalInfo Collector::collect_thermal() const {
  return {};
}

ThermalInfo collect_thermal() { return Collector{}.collect_thermal();}

}