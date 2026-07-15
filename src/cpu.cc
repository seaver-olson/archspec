#include "aview.hh"
#include "internal.hh"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <thread>

#if !defined(_WIN32)
#include <sys/utsname.h>
#endif

namespace archspec {
namespace {

ArchType arch_from_machine(const std::string& machine) {
  if (machine == "x86_64" || machine == "amd64") {
    return ArchType::x86_64;
  }
  if (machine == "i386" || machine == "i486" || machine == "i586" || machine == "i686") {
    return ArchType::x86;
  }
  if (machine == "armv7l" || machine == "armv8l" || detail::starts_with(machine, "armv")) {
    return ArchType::arm;
  }
  if (machine == "aarch64" || machine == "arm64") {
    return ArchType::aarch64;
  }
  if (machine == "riscv32") {
    return ArchType::riscv32;
  }
  if (machine == "riscv64") {
    return ArchType::riscv64;
  }
  if (machine == "ppc64" || machine == "ppc64le") {
    return ArchType::ppc64;
  }
  if (machine == "s390x") {
    return ArchType::s390x;
  }

  return ArchType::unknown;
}

std::vector<std::pair<std::uint64_t, std::string>> cpu_dirs(const CollectOptions& options) {
  std::vector<std::pair<std::uint64_t, std::string>> cpus;
  std::string root = detail::sys_path(options, "/devices/system/cpu");

  for (const std::string& name : detail::list_dir(root)) {
    if (!detail::starts_with(name, "cpu") || name.size() == 3) {
      continue;
    }

    std::uint64_t id = 0;
    if (!detail::parse_u64(name.substr(3), id) || id > UINT32_MAX) {
      continue;
    }

    cpus.push_back({id, detail::join_path(root, name)});
  }

  std::sort(cpus.begin(), cpus.end());
  return cpus;
}

std::optional<std::uint32_t> u32_from_text(const std::string& text) {
  std::uint64_t value = 0;
  if (!detail::parse_u64(text, value) || value > UINT32_MAX) {
    return std::nullopt;
  }

  return static_cast<std::uint32_t>(value);
}

std::optional<std::uint32_t> u32_from_map(
    const detail::KeyValueMap& block,
    const std::string& key
) {
  auto value = detail::map_value(block, key);
  if (!value) {
    return std::nullopt;
  }

  return u32_from_text(*value);
}

void parse_address_sizes(
    const detail::KeyValueMap& block,
    U64Field& physical,
    U64Field& virtual_bits
) {
  auto text = detail::map_value(block, "address sizes");
  if (!text) {
    physical = U64Field::unavailable(Status::not_found);
    virtual_bits = U64Field::unavailable(Status::not_found);
    return;
  }

  std::istringstream in(*text);
  std::string word;
  std::uint64_t last_number = 0;
  bool have_number = false;
  bool found_physical = false;
  bool found_virtual = false;

  while (in >> word) {
    std::uint64_t parsed = 0;
    if (detail::parse_u64(word, parsed)) {
      last_number = parsed;
      have_number = true;
      continue;
    }

    if (have_number && detail::starts_with(word, "physical")) {
      physical = U64Field::value(last_number);
      found_physical = true;
    } else if (have_number && detail::starts_with(word, "virtual")) {
      virtual_bits = U64Field::value(last_number);
      found_virtual = true;
    }
  }

  if (!found_physical) {
    physical = U64Field::unavailable(Status::parse_error);
  }
  if (!found_virtual) {
    virtual_bits = U64Field::unavailable(Status::parse_error);
  }
}

std::string first_flags(const detail::KeyValueMap& block) {
  for (const char* key : {"flags", "Features", "isa"}) {
    auto value = detail::map_value(block, key);
    if (value) {
      return *value;
    }
  }

  return {};
}

StringField first_existing_hypervisor_vendor(const CollectOptions& options) {
  for (const std::string& path : {
           detail::sys_path(options, "/hypervisor/type"),
           detail::sys_path(options, "/class/dmi/id/product_name"),
           detail::sys_path(options, "/class/dmi/id/sys_vendor"),
       }) {
    StringField field = detail::read_string_field(path);
    if (field.valid() && !field.value().empty()) {
      return field;
    }
  }

  return StringField::unavailable(Status::not_found);
}

std::map<std::uint64_t, detail::KeyValueMap> blocks_by_processor(
    const std::vector<detail::KeyValueMap>& blocks
) {
  std::map<std::uint64_t, detail::KeyValueMap> by_id;

  for (const detail::KeyValueMap& block : blocks) {
    auto processor = detail::map_value(block, "processor");
    std::uint64_t id = 0;
    if (processor && detail::parse_u64(*processor, id)) {
      by_id[id] = block;
    }
  }

  return by_id;
}

void add_cpuinfo_topology(
    const std::vector<detail::KeyValueMap>& blocks,
    std::vector<CpuTopologyEntry>& entries
) {
  for (const detail::KeyValueMap& block : blocks) {
    auto processor = u32_from_map(block, "processor");
    if (!processor) {
      continue;
    }

    CpuTopologyEntry entry;
    entry.logical_id = U32Field::value(*processor);

    if (auto core = u32_from_map(block, "core id")) {
      entry.core_id = U32Field::value(*core);
    }
    if (auto package = u32_from_map(block, "physical id")) {
      entry.package_id = U32Field::value(*package);
    }

    entries.push_back(entry);
  }
}

} // namespace

CpuInfo Collector::collect_cpu() const {
  CpuInfo info;
  info.arch = ArchType::unknown;

#if defined(_WIN32)
#if defined(_M_X64) || defined(__x86_64__)
  std::string machine = "x86_64";
#elif defined(_M_IX86) || defined(__i386__)
  std::string machine = "x86";
#elif defined(_M_ARM64) || defined(__aarch64__)
  std::string machine = "aarch64";
#elif defined(_M_ARM) || defined(__arm__)
  std::string machine = "arm";
#else
  std::string machine = "unknown";
#endif
  info.arch_name = StringField::value(machine);
  info.arch = arch_from_machine(machine);
#else
  struct utsname uts {};
  if (uname(&uts) == 0) {
    std::string machine = uts.machine;
    info.arch_name = StringField::value(machine);
    info.arch = arch_from_machine(machine);
  } else {
    info.arch_name = StringField::unavailable(Status::internal_error);
  }
#endif

  detail::ReadResult cpuinfo = detail::read_file(detail::proc_path(options_, "/cpuinfo"));
  std::vector<detail::KeyValueMap> blocks;
  if (cpuinfo.status == Status::ok) {
    blocks = detail::parse_colon_blocks(cpuinfo.value);
  }

  const detail::KeyValueMap empty;
  const detail::KeyValueMap& first = blocks.empty() ? empty : blocks.front();

  info.vendor = detail::string_from_map(first, {"vendor_id", "CPU implementer", "uarch"});
  info.model_name = detail::string_from_map(first, {"model name", "Processor", "cpu"});
  info.brand = info.model_name;

  info.family = detail::u64_from_map(first, {"cpu family", "CPU architecture"});
  info.model = detail::u64_from_map(first, {"model", "CPU part"});
  info.stepping = detail::u64_from_map(first, {"stepping", "CPU revision"});
  info.microcode = detail::string_from_map(first, {"microcode"});

  parse_address_sizes(first, info.physical_address_bits, info.virtual_address_bits);

  info.present_cpu_count = detail::read_cpu_list_count_field(
      detail::sys_path(options_, "/devices/system/cpu/present")
  );
  info.online_cpu_count = detail::read_cpu_list_count_field(
      detail::sys_path(options_, "/devices/system/cpu/online")
  );
  info.possible_cpu_count = detail::read_cpu_list_count_field(
      detail::sys_path(options_, "/devices/system/cpu/possible")
  );

  if (!info.present_cpu_count.valid() && !blocks.empty()) {
    info.present_cpu_count = U64Field::value(blocks.size());
  }
  if (!info.online_cpu_count.valid() && !blocks.empty()) {
    info.online_cpu_count = U64Field::value(blocks.size());
  }
  if (!info.possible_cpu_count.valid() && !blocks.empty()) {
    info.possible_cpu_count = U64Field::value(blocks.size());
  }
  const unsigned int hardware_threads = std::thread::hardware_concurrency();
  if (!info.present_cpu_count.valid() && hardware_threads != 0) {
    info.present_cpu_count = U64Field::value(hardware_threads);
  }
  if (!info.online_cpu_count.valid() && hardware_threads != 0) {
    info.online_cpu_count = U64Field::value(hardware_threads);
  }
  if (!info.possible_cpu_count.valid() && hardware_threads != 0) {
    info.possible_cpu_count = U64Field::value(hardware_threads);
  }

  std::map<std::uint64_t, detail::KeyValueMap> cpuinfo_by_id = blocks_by_processor(blocks);
  for (const auto& cpu : cpu_dirs(options_)) {
    CpuTopologyEntry entry;
    entry.logical_id = U32Field::value(static_cast<std::uint32_t>(cpu.first));

    std::string topology_dir = detail::join_path(cpu.second, "topology");
    entry.core_id = detail::read_u32_field(detail::join_path(topology_dir, "core_id"));
    entry.package_id = detail::read_u32_field(
        detail::join_path(topology_dir, "physical_package_id")
    );
    entry.thread_siblings_mask = detail::read_string_field(
        detail::join_path(topology_dir, "thread_siblings")
    );
    entry.core_siblings_mask = detail::read_string_field(
        detail::join_path(topology_dir, "core_siblings")
    );

    for (const std::string& child : detail::list_dir(cpu.second)) {
      if (!detail::starts_with(child, "node")) {
        continue;
      }

      std::uint64_t node = 0;
      if (detail::parse_u64(child.substr(4), node) && node <= UINT32_MAX) {
        entry.numa_node = U32Field::value(static_cast<std::uint32_t>(node));
        break;
      }
    }

    auto cpuinfo_block = cpuinfo_by_id.find(cpu.first);
    if (cpuinfo_block != cpuinfo_by_id.end()) {
      if (!entry.core_id.valid()) {
        if (auto core = u32_from_map(cpuinfo_block->second, "core id")) {
          entry.core_id = U32Field::value(*core);
        }
      }
      if (!entry.package_id.valid()) {
        if (auto package = u32_from_map(cpuinfo_block->second, "physical id")) {
          entry.package_id = U32Field::value(*package);
        }
      }
    }

    info.topology.push_back(entry);
  }

  if (info.topology.empty() && !blocks.empty()) {
    add_cpuinfo_topology(blocks, info.topology);
  }

  std::set<std::uint32_t> packages;
  std::map<std::uint32_t, std::set<std::uint32_t>> cores_by_package;
  std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint64_t> threads_by_core;

  for (const CpuTopologyEntry& entry : info.topology) {
    if (!entry.package_id.valid() || !entry.core_id.valid()) {
      continue;
    }

    std::uint32_t package = entry.package_id.value();
    std::uint32_t core = entry.core_id.value();
    packages.insert(package);
    cores_by_package[package].insert(core);
    threads_by_core[{package, core}]++;
  }

  if (!packages.empty()) {
    info.sockets = U64Field::value(packages.size());
  } else {
    info.sockets = U64Field::unavailable(Status::not_found);
  }

  auto cpu_cores = detail::u64_from_map(first, {"cpu cores"});
  auto siblings = detail::u64_from_map(first, {"siblings"});
  if (cpu_cores.valid()) {
    info.cores_per_socket = cpu_cores;
  } else if (!cores_by_package.empty()) {
    std::uint64_t max_cores = 0;
    for (const auto& item : cores_by_package) {
      max_cores = std::max<std::uint64_t>(max_cores, item.second.size());
    }
    info.cores_per_socket = U64Field::value(max_cores);
  } else {
    info.cores_per_socket = U64Field::unavailable(Status::not_found);
  }

  if (siblings.valid() && cpu_cores.valid() && cpu_cores.value() != 0) {
    info.threads_per_core = U64Field::value(siblings.value() / cpu_cores.value());
  } else if (!threads_by_core.empty()) {
    std::uint64_t max_threads = 0;
    for (const auto& item : threads_by_core) {
      max_threads = std::max(max_threads, item.second);
    }
    info.threads_per_core = U64Field::value(max_threads);
  } else {
    info.threads_per_core = U64Field::unavailable(Status::not_found);
  }

  std::set<std::string> flags = detail::word_set(first_flags(first));
  if (!flags.empty()) {
    info.is_virtualized = BoolField::value(flags.find("hypervisor") != flags.end());
  } else {
    info.is_virtualized = BoolField::unavailable(
        cpuinfo.status == Status::ok ? Status::not_found : cpuinfo.status
    );
  }

  auto hypervisor_vendor = detail::string_from_map(first, {"Hypervisor vendor"});
  if (hypervisor_vendor.valid()) {
    info.hypervisor_vendor = hypervisor_vendor;
  } else if (info.is_virtualized.valid() && info.is_virtualized.value()) {
    info.hypervisor_vendor = first_existing_hypervisor_vendor(options_);
  } else {
    info.hypervisor_vendor = StringField::unavailable(Status::not_found);
  }

  return info;
}

CpuInfo collect_cpu() { return Collector{}.collect_cpu(); }

} // namespace archspec
