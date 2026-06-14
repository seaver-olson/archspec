#include "aview.hh"

namespace archspec {

PlatformInfo Collector::collect_platform() const {
  return {};
}
PlatformInfo collect_platform() { return Collector{}.collect_platform();}

}