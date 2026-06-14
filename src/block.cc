#include "aview.hh"

namespace archspec {

BlockDeviceList Collector::collect_block() const {
  return {};
}

BlockDeviceList collect_block() { return Collector{}.collect_block();}

}