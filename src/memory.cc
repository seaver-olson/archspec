#include "aview.hh"

#include <fstream>
#include <unistd.h>

namespace archspec {

MemoryInfo Collector::collect_memory() const {

  MemoryInfo info;

  std::ifstream file("/proc/meminfo");
  if (!file.is_open()) {
    info.total_kb = U64Field::unavailable(Status::not_found);
    return info;
  }

  std::string key;
  std::uint64_t value = 0;
  std::string unit;

  while (file >> key >> value >> unit) {
    if (key == "MemTotal:") {
      info.total_kb = U64Field::value(value);
    } else if (key == "MemFree:") {
      info.free_kb = U64Field::value(value);
    } else if (key == "MemAvailable:") {
      info.available_kb = U64Field::value(value);
    } else if (key == "Buffers:") {
      info.buffers_kb = U64Field::value(value);
    } else if (key == "Cached:") {
      info.cached_kb = U64Field::value(value);
    } else if (key == "SwapTotal:") {
      info.swap_total_kb = U64Field::value(value);
    } else if (key == "SwapFree:") {
      info.swap_free_kb = U64Field::value(value);
    } else if (key == "Hugepagesize:") {
      info.hugepage_size_kb = U64Field::value(value);
    } else if (key == "HugePages_Total:") {
      info.hugepages_total = U64Field::value(value);
    } else if (key == "HugePages_Free:") {
      info.hugepages_free = U64Field::value(value);
    }
  }

  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    info.page_size_bytes = U64Field::value(static_cast<std::uint64_t>(page));
  } else {
    info.page_size_bytes = U64Field::unavailable(Status::not_found);
  }

  info.transparent_hugepages_enabled = BoolField::unavailable(Status::unsupported);

  return info;
}

MemoryInfo collect_memory() { return Collector{}.collect_memory();}

}