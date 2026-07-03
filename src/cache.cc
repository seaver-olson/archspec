#include "aview.hh"
#include "internal.hh"

#include <algorithm>
#include <set>
#include <sstream>

namespace archspec {
namespace {

CacheType cache_type_from_string(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  if (value == "data") {
    return CacheType::data;
  }
  if (value == "instruction") {
    return CacheType::instruction;
  }
  if (value == "unified") {
    return CacheType::unified;
  }

  return CacheType::unknown;
}

std::vector<std::pair<std::uint64_t, std::string>> cpu_dirs(const CollectOptions& options) {
  std::vector<std::pair<std::uint64_t, std::string>> cpus;
  std::string root = detail::sys_path(options, "/devices/system/cpu");

  for (const std::string& name : detail::list_dir(root)) {
    if (!detail::starts_with(name, "cpu") || name.size() == 3) {
      continue;
    }

    std::uint64_t id = 0;
    if (detail::parse_u64(name.substr(3), id)) {
      cpus.push_back({id, detail::join_path(root, name)});
    }
  }

  std::sort(cpus.begin(), cpus.end());
  return cpus;
}

std::string cache_key(const CacheInfo& cache) {
  std::ostringstream out;
  out << (cache.level.valid() ? std::to_string(cache.level.value()) : "?")
      << "|" << to_string(cache.type)
      << "|" << (cache.size.valid() ? std::to_string(cache.size.value()) : "?")
      << "|" << (cache.shared_cpu_list.valid() ? cache.shared_cpu_list.value() : "?");
  return out.str();
}

} // namespace

CacheList Collector::collect_cache() const {
  CacheList list;
  std::set<std::string> seen;

  for (const auto& cpu : cpu_dirs(options_)) {
    std::string cache_root = detail::join_path(cpu.second, "cache");
    for (const std::string& index : detail::list_dir(cache_root)) {
      if (!detail::starts_with(index, "index")) {
        continue;
      }

      std::string path = detail::join_path(cache_root, index);
      CacheInfo cache;
      cache.cpu_id = U32Field::value(static_cast<std::uint32_t>(cpu.first));
      cache.level = detail::read_u32_field(detail::join_path(path, "level"));
      cache.size = detail::read_size_bytes_field(detail::join_path(path, "size"));
      cache.line_size = detail::read_u64_field(
          detail::join_path(path, "coherency_line_size")
      );
      cache.associativity = detail::read_u64_field(
          detail::join_path(path, "ways_of_associativity")
      );
      cache.sets = detail::read_u64_field(detail::join_path(path, "number_of_sets"));
      cache.shared_cpu_list = detail::read_string_field(
          detail::join_path(path, "shared_cpu_list")
      );

      StringField type = detail::read_string_field(detail::join_path(path, "type"));
      if (type.valid()) {
        cache.type = cache_type_from_string(type.value());
      }

      std::string key = cache_key(cache);
      if (seen.insert(key).second) {
        list.entries.push_back(cache);
      }
    }
  }

  return list;
}

CacheList collect_cache() { return Collector{}.collect_cache(); }

} // namespace archspec
