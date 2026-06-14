#include "aview.hh"

namespace archspec {

CacheList Collector::collect_cache() const {
  return {};
}


CacheList collect_cache() { return Collector{}.collect_cache();}

}