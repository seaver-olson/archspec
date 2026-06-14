#include "aview.hh"

namespace archspec {

PerfCounterInfo Collector::collect_perf() const {
  return {};
}

PerfCounterInfo collect_perf() { return Collector{}.collect_perf();}

}