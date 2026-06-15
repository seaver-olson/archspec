#include "aview.hh"

#include <cstring>
#include <iostream>
#include <string>

namespace {

template <typename T>
void print_field(const std::string& label, const archspec::Field<T>& field) {
  std::cout << label << ": ";

  if (field.valid()) {
    std::cout << field.value();
  } else {
    std::cout << "unavailable (" << archspec::to_string(field.status()) << ")";
  }

  std::cout << '\n';
}

void print_summary(const archspec::SystemInfo& info) {
  std::cout << "ArchSpec Summary\n";
  std::cout << "================\n\n";

  std::cout << "OS\n";
  print_field("  Kernel name", info.os_info.kernel_name);
  print_field("  Kernel release", info.os_info.kernel_release);
  print_field("  Kernel version", info.os_info.kernel_version);
  print_field("  Machine", info.os_info.machine);
  print_field("  Hostname", info.os_info.hostname);
  print_field("  Distro", info.os_info.distro_name);
  print_field("  Distro version", info.os_info.distro_version);
  print_field("  Page size", info.os_info.page_size);
  print_field("  Word size", info.os_info.word_size);
  print_field("  Endianness", info.os_info.endianness);

  std::cout << "\nCPU\n";
  std::cout << "  Architecture enum: " << archspec::to_string(info.cpu_info.arch) << '\n';
  print_field("  Architecture name", info.cpu_info.arch_name);
  print_field("  Vendor", info.cpu_info.vendor);
  print_field("  Brand", info.cpu_info.brand);
  print_field("  Model name", info.cpu_info.model_name);
  print_field("  Present CPUs", info.cpu_info.present_cpu_count);
  print_field("  Online CPUs", info.cpu_info.online_cpu_count);
  print_field("  Possible CPUs", info.cpu_info.possible_cpu_count);
  print_field("  Physical address bits", info.cpu_info.physical_address_bits);
  print_field("  Virtual address bits", info.cpu_info.virtual_address_bits);
  print_field("  Virtualized", info.cpu_info.is_virtualized);

  std::cout << "\nMemory\n";
  print_field("  Total KiB", info.memory_info.total_kb);
  print_field("  Free KiB", info.memory_info.free_kb);
  print_field("  Available KiB", info.memory_info.available_kb);
  print_field("  Buffers KiB", info.memory_info.buffers_kb);
  print_field("  Cached KiB", info.memory_info.cached_kb);
  print_field("  Swap total KiB", info.memory_info.swap_total_kb);
  print_field("  Swap free KiB", info.memory_info.swap_free_kb);
  print_field("  Page size bytes", info.memory_info.page_size_bytes);
  print_field("  Hugepage size KiB", info.memory_info.hugepage_size_kb);
}

} // namespace

int main(int argc, char** argv) {
  archspec::CollectOptions options;

  options.categories =
      archspec::CollectCategory::os |
      archspec::CollectCategory::cpu |
      archspec::CollectCategory::memory;

  archspec::SystemInfo info = archspec::collect_system(options);

  if (argc > 1 && std::strcmp(argv[1], "--json") == 0) {
    std::cout << archspec::to_json(info) << '\n';
    return 0;
  }

  print_summary(info);
  return 0;
}