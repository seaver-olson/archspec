#include "aview.hh"

#include <fstream>
#include <unistd.h>
#include <sstream>

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

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);

    std::string key;
    std::uint64_t value = 0;

    if (!(iss >> key >> value)) {
      continue;
    }

    if (key == "MemTotal:") {
      info.total_kb = U64Field::value(value);
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