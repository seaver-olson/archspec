#include <archspec/archspec.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct CategoryEntry {
  const char* name;
  archspec::CollectCategory value;
};

constexpr CategoryEntry categories[] = {
    {"os", archspec::CollectCategory::os},
    {"cpu", archspec::CollectCategory::cpu},
    {"isa", archspec::CollectCategory::isa},
    {"cache", archspec::CollectCategory::cache},
    {"memory", archspec::CollectCategory::memory},
    {"pci", archspec::CollectCategory::pci},
    {"gpu", archspec::CollectCategory::gpu},
    {"perf", archspec::CollectCategory::perf},
    {"block", archspec::CollectCategory::block},
    {"net", archspec::CollectCategory::net},
    {"thermal", archspec::CollectCategory::thermal},
    {"power", archspec::CollectCategory::power},
    {"virtualization", archspec::CollectCategory::virtualization},
    {"platform", archspec::CollectCategory::platform},
};

void print_help(std::ostream& out) {
  out <<
      "archspec-inspect " << archspec::version << "\n"
      "Collect a safe, structured description of the local system.\n\n"
      "Usage: archspec-inspect [options]\n\n"
      "  --format text|json       Output format (default: text)\n"
      "  --json                   Alias for --format json\n"
      "  --categories LIST        Comma-separated categories or 'all'\n"
      "  --include-sensitive      Include hostname, command line, and addresses\n"
      "  --redact                 Hide sensitive fields (the default)\n"
      "  --allow-perf             Permit perf_event_open capability probes\n"
      "  --allow-slow             Permit opt-in slow probes\n"
      "  --allow-vendor-libs      Permit opt-in vendor library probes\n"
      "  --procfs-root PATH       Read Linux procfs data below PATH\n"
      "  --sysfs-root PATH        Read Linux sysfs data below PATH\n"
      "  --etc-root PATH          Read operating-system metadata below PATH\n"
      "  --dev-root PATH          Resolve device paths below PATH\n"
      "  --capabilities           Print the machine-readable CLI contract\n"
      "  --version                Print the program version\n"
      "  --help                   Show this help\n\n"
      "Categories: os,cpu,isa,cache,memory,pci,gpu,perf,block,net,thermal,"
      "power,virtualization,platform\n";
}

bool parse_categories(
    const std::string& text,
    archspec::CollectCategory& selected,
    std::string& error
) {
  if (text == "all") {
    selected = archspec::CollectCategory::all;
    return true;
  }
  if (text == "none" || text.empty()) {
    error = "category list must contain at least one category";
    return false;
  }

  selected = archspec::CollectCategory::none;
  std::istringstream input(text);
  std::string name;
  while (std::getline(input, name, ',')) {
    bool found = false;
    for (const CategoryEntry& entry : categories) {
      if (name == entry.name) {
        selected = selected | entry.value;
        found = true;
        break;
      }
    }
    if (!found) {
      error = "unknown category '" + name + "'";
      return false;
    }
  }
  return true;
}

template <typename T>
void print_field(const char* label, const archspec::Field<T>& field) {
  std::cout << "  " << label << ": ";
  if (field.valid()) {
    std::cout << field.value();
  } else {
    std::cout << "unavailable (" << archspec::status_code(field.status()) << ")";
  }
  std::cout << '\n';
}

void print_text(const archspec::Report& report) {
  const auto& info = report.system;
  std::cout << "ArchSpec Inspect " << archspec::version << "\n";

  if (archspec::has_category(report.categories, archspec::CollectCategory::os)) {
    std::cout << "\nOS\n";
    print_field("kernel", info.os_info.kernel_name);
    print_field("kernel release", info.os_info.kernel_release);
    print_field("distribution", info.os_info.distro_name);
    print_field("hostname", info.os_info.hostname);
  }
  if (archspec::has_category(report.categories, archspec::CollectCategory::cpu)) {
    std::cout << "\nCPU\n";
    std::cout << "  architecture: " << archspec::to_string(info.cpu_info.arch) << '\n';
    print_field("model", info.cpu_info.model_name);
    print_field("online CPUs", info.cpu_info.online_cpu_count);
    print_field("sockets", info.cpu_info.sockets);
    print_field("cores per socket", info.cpu_info.cores_per_socket);
    print_field("threads per core", info.cpu_info.threads_per_core);
  }
  if (archspec::has_category(report.categories, archspec::CollectCategory::memory)) {
    std::cout << "\nMemory\n";
    print_field("total KiB", info.memory_info.total_kb);
    print_field("available KiB", info.memory_info.available_kb);
    print_field("NUMA nodes", archspec::U64Field::value(info.memory_info.numa_nodes.size()));
  }

  struct CountEntry {
    archspec::CollectCategory category;
    const char* label;
    std::size_t count;
  };
  const CountEntry counts[] = {
      {archspec::CollectCategory::cache, "cache entries", info.cache_list.entries.size()},
      {archspec::CollectCategory::pci, "PCI devices", info.pci_devices.entries.size()},
      {archspec::CollectCategory::gpu, "GPUs", info.gpu_list.entries.size()},
      {archspec::CollectCategory::block, "block devices", info.block_devices.entries.size()},
      {archspec::CollectCategory::net, "network interfaces", info.net_interfaces.entries.size()},
      {archspec::CollectCategory::thermal, "thermal zones", info.thermal_info.entries.size()},
  };
  for (const CountEntry& entry : counts) {
    if (archspec::has_category(report.categories, entry.category)) {
      std::cout << "\n" << entry.label << ": " << entry.count << '\n';
    }
  }
}

void print_capabilities() {
  std::cout
      << "{\"name\":\"archspec-inspect\",\"version\":\"" << archspec::version
      << "\",\"report_schema_version\":\"" << archspec::report_schema_version
      << "\",\"formats\":[\"text\",\"json\"],\"categories\":[";
  for (std::size_t i = 0; i < sizeof(categories) / sizeof(categories[0]); ++i) {
    if (i != 0) {
      std::cout << ',';
    }
    std::cout << '\"' << categories[i].name << '\"';
  }
  std::cout << "],\"safe_by_default\":true,\"supports_redaction\":true}\n";
}

bool take_value(
    int& index,
    int argc,
    char** argv,
    const std::string& option,
    std::string& value
) {
  if (index + 1 >= argc) {
    std::cerr << "archspec-inspect: " << option << " requires a value\n";
    return false;
  }
  value = argv[++index];
  return true;
}

} // namespace

int main(int argc, char** argv) {
  archspec::CollectOptions options;
  options.include_sensitive = false;
  std::string format = "text";

  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    std::string value;

    if (argument == "--help" || argument == "-h") {
      print_help(std::cout);
      return 0;
    }
    if (argument == "--version") {
      std::cout << "archspec-inspect " << archspec::version << '\n';
      return 0;
    }
    if (argument == "--capabilities") {
      print_capabilities();
      return 0;
    }
    if (argument == "--json") {
      format = "json";
      continue;
    }
    if (argument == "--redact") {
      options.include_sensitive = false;
      continue;
    }
    if (argument == "--include-sensitive") {
      options.include_sensitive = true;
      continue;
    }
    if (argument == "--allow-perf") {
      options.allow_perf_open = true;
      continue;
    }
    if (argument == "--allow-slow") {
      options.allow_slow_probes = true;
      continue;
    }
    if (argument == "--allow-vendor-libs") {
      options.allow_vendor_libraries = true;
      continue;
    }

    if (argument == "--format") {
      if (!take_value(i, argc, argv, argument, value)) return 2;
      if (value != "text" && value != "json") {
        std::cerr << "archspec-inspect: unknown format '" << value << "'\n";
        return 2;
      }
      format = value;
      continue;
    }
    if (argument == "--categories") {
      if (!take_value(i, argc, argv, argument, value)) return 2;
      std::string error;
      if (!parse_categories(value, options.categories, error)) {
        std::cerr << "archspec-inspect: " << error << '\n';
        return 2;
      }
      continue;
    }

    std::string* root = nullptr;
    if (argument == "--procfs-root") root = &options.procfs_root;
    if (argument == "--sysfs-root") root = &options.sysfs_root;
    if (argument == "--etc-root") root = &options.etc_root;
    if (argument == "--dev-root") root = &options.dev_root;
    if (root != nullptr) {
      if (!take_value(i, argc, argv, argument, *root)) return 2;
      continue;
    }

    std::cerr << "archspec-inspect: unknown option '" << argument
              << "' (try --help)\n";
    return 2;
  }

  const archspec::Report report = archspec::collect_report(options);
  if (format == "json") {
    std::cout << archspec::to_json(report) << '\n';
  } else {
    print_text(report);
  }
  return 0;
}
