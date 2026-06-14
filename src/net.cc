#include "aview.hh"

namespace archspec {

NetInterfaceList Collector::collect_net() const {
  return {};
}

NetInterfaceList collect_net() { return Collector{}.collect_net();}

}