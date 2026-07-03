#include "aview.hh"
#include "internal.hh"

#include <map>
#include <sstream>
#include <unistd.h>

namespace archspec {
namespace {

std::map<std::string, std::uint64_t> parse_meminfo_values(const std::string& text) {
  std::map<std::string, std::uint64_t> values;
  std::istringstream in(text);
  std::string line;

  while (std::getline(in, line)) {
    std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }

    std::string key = detail::trim(line.substr(0, colon));
    std::istringstream value_stream(line.substr(colon + 1));
    std::uint64_t value = 0;
    if (value_stream >> value) {
      values[key] = value;
    }
  }

  return values;
}

U64Field meminfo_field(
    const std::map<std::string, std::uint64_t>& values,
    const std::string& key
) {
  auto it = values.find(key);
  if (it == values.end()) {
    return U64Field::unavailable(Status::not_found);
  }

  return U64Field::value(it->second);
}

BoolField transparent_hugepages_enabled(const CollectOptions& options) {
  detail::ReadResult result = detail::read_first_line(
      detail::sys_path(options, "/kernel/mm/transparent_hugepage/enabled")
  );
  if (result.status != Status::ok) {
    return BoolField::unavailable(result.status);
  }

  std::string value = result.value;
  std::size_t open = value.find('[');
  std::size_t close = value.find(']', open == std::string::npos ? 0 : open);
  if (open != std::string::npos && close != std::string::npos && close > open + 1) {
    std::string selected = value.substr(open + 1, close - open - 1);
    return BoolField::value(selected != "never");
  }

  return BoolField::value(value.find("never") == std::string::npos);
}

std::vector<NumaNodeInfo> collect_numa_nodes(const CollectOptions& options) {
  std::vector<NumaNodeInfo> nodes;
  std::string root = detail::sys_path(options, "/devices/system/node");

  for (const std::string& name : detail::list_dir(root)) {
    if (!detail::starts_with(name, "node")) {
      continue;
    }

    std::uint64_t id = 0;
    if (!detail::parse_u64(name.substr(4), id) || id > UINT32_MAX) {
      continue;
    }

    std::string node_path = detail::join_path(root, name);
    NumaNodeInfo node;
    node.node_id = U32Field::value(static_cast<std::uint32_t>(id));
    node.cpu_list = detail::read_string_field(detail::join_path(node_path, "cpulist"));

    detail::ReadResult meminfo = detail::read_file(detail::join_path(node_path, "meminfo"));
    if (meminfo.status == Status::ok) {
      std::map<std::string, std::uint64_t> values = parse_meminfo_values(meminfo.value);
      node.total_kb = meminfo_field(values, "Node " + std::to_string(id) + " MemTotal");
      node.free_kb = meminfo_field(values, "Node " + std::to_string(id) + " MemFree");
    } else {
      node.total_kb = U64Field::unavailable(meminfo.status);
      node.free_kb = U64Field::unavailable(meminfo.status);
    }

    nodes.push_back(node);
  }

  return nodes;
}

} // namespace

MemoryInfo Collector::collect_memory() const {
  MemoryInfo info;

  detail::ReadResult meminfo = detail::read_file(detail::proc_path(options_, "/meminfo"));
  if (meminfo.status == Status::ok) {
    std::map<std::string, std::uint64_t> values = parse_meminfo_values(meminfo.value);

    info.total_kb = meminfo_field(values, "MemTotal");
    info.free_kb = meminfo_field(values, "MemFree");
    info.available_kb = meminfo_field(values, "MemAvailable");
    info.buffers_kb = meminfo_field(values, "Buffers");
    info.cached_kb = meminfo_field(values, "Cached");
    info.swap_total_kb = meminfo_field(values, "SwapTotal");
    info.swap_free_kb = meminfo_field(values, "SwapFree");
    info.hugepage_size_kb = meminfo_field(values, "Hugepagesize");
    info.hugepages_total = meminfo_field(values, "HugePages_Total");
    info.hugepages_free = meminfo_field(values, "HugePages_Free");
  } else {
    info.total_kb = U64Field::unavailable(meminfo.status);
    info.free_kb = U64Field::unavailable(meminfo.status);
    info.available_kb = U64Field::unavailable(meminfo.status);
    info.buffers_kb = U64Field::unavailable(meminfo.status);
    info.cached_kb = U64Field::unavailable(meminfo.status);
    info.swap_total_kb = U64Field::unavailable(meminfo.status);
    info.swap_free_kb = U64Field::unavailable(meminfo.status);
    info.hugepage_size_kb = U64Field::unavailable(meminfo.status);
    info.hugepages_total = U64Field::unavailable(meminfo.status);
    info.hugepages_free = U64Field::unavailable(meminfo.status);
  }

  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    info.page_size_bytes = U64Field::value(static_cast<std::uint64_t>(page));
  } else {
    info.page_size_bytes = U64Field::unavailable(Status::not_found);
  }

  info.transparent_hugepages_enabled = transparent_hugepages_enabled(options_);
  info.numa_nodes = collect_numa_nodes(options_);

  return info;
}

MemoryInfo collect_memory() { return Collector{}.collect_memory(); }

} // namespace archspec
