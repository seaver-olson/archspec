#include "aview.hh"

namespace archspec {

VirtualizationInfo Collector::collect_virtualization() const {
  return {};
}
VirtualizationInfo collect_virtualization() { return Collector{}.collect_virtualization();}

}