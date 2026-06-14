#include "aview.hh"

namespace archspec {
  
PciDeviceList Collector::collect_pci() const {
  return {};
}

PciDeviceList collect_pci() { return Collector{}.collect_pci();}

}