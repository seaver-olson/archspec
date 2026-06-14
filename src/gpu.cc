#include "aview.hh"

namespace archspec {

GpuList Collector::collect_gpu() const {
  return {};
}

GpuList collect_gpu() { return Collector{}.collect_gpu();}

}