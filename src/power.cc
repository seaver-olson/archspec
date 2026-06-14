#include "aview.hh"

namespace archspec {

PowerInfo Collector::collect_power() const {
  return {};
}
PowerInfo collect_power() { return Collector{}.collect_power();}

}