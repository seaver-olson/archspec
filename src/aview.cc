#include "aview.hh"

#include <sys/utsname.h>
#include <unistd.h>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace archspec {

std::string to_string(Status status) {
  switch (status){
    case Status::ok: return "ok";
    case Status::unsupported: return "unsupported";
    case Status::perm_denied: return "permission denied";
    case Status::not_found: return "not found";
    case Status::parse_error: return "parse error";
    case Status::invalid_arg: return "invalid argument";
    case Status::internal_error: return "internal error";
    default: return "unknown";
  }
}

std::string to_string(ArchType arch) {
  switch (arch){
    case ArchType::unknown: return "unknown";
    case ArchType::x86: return "x86";
    case ArchType::x86_64: return "x86_64";
    case ArchType::arm: return "arm";
    case ArchType::aarch64: return "aarch64";
    case ArchType::riscv32: return "riscv32";
    case ArchType::riscv64: return "riscv64";
    case ArchType::ppc64: return "ppc64";
    case ArchType::s390x: return "s390x";
    default: return "unknown";
  }
}

std::string to_string(CacheType type) {
  switch (type){
    case CacheType::unknown: return "unknown";
    case CacheType::data: return "data";
    case CacheType::instruction: return "instruction";
    case CacheType::unified: return "unified";
    default: return "unknown";
  }
}

std::string to_string(GpuVendor vendor) {
  switch (vendor){
    case GpuVendor::unknown: return "unknown";
    case GpuVendor::intel: return "intel";
    case GpuVendor::amd: return "amd";
    case GpuVendor::nvidia: return "nvidia";
    default: return "unknown";
  }
}

bool available(Status status) {
  return status == Status::ok;
}

//private helpers
namespace {
std::string read_first_line(const std::string& path){
  std::ifstream file(path);
  std::string line;

  if (!file.is_open()){
    return {};
  }

  std::getline(file, line);
  return line;
}

ArchType arch_from_machine(const std::string& machine){
  if (machine == "x86_64" || machine == "amd64"){
    return ArchType::x86_64;
  } else if (machine == "i386" || machine == "i686"){
    return ArchType::x86;
  } else if (machine == "armv7l" || machine == "armv8l"){
    return ArchType::arm;
  } else if (machine == "aarch64"){
    return ArchType::aarch64;
  } else if (machine == "riscv32"){
    return ArchType::riscv32;
  } else if (machine == "riscv64"){
    return ArchType::riscv64;
  } else if (machine == "ppc64le"){
    return ArchType::ppc64;
  } else if (machine == "s390x"){
    return ArchType::s390x;
  }

  return ArchType::unknown;
}

std::uint64_t count_cpu_range_list(const std::string& text){

}

} // end of private helpers

Collector::Collector(CollectOptions options) : options_(options) {}

const CollectOptions& Collector::options() const {
  return options_;
}

void Collector::set_options(const CollectOptions& options) {
  options_ = options;
}

SystemInfo Collector::collect() const {
  SystemInfo info;

  if (has_category(options_.categories, CollectCategory::os)){ info.os_info = collect_os();}
  if (has_category(options_.categories, CollectCategory::cpu)){ info.cpu_info = collect_cpu();}
  if (has_category(options_.categories, CollectCategory::isa)){ info.isa_features = collect_isa();}
  if (has_category(options_.categories, CollectCategory::cache)){ info.cache_list = collect_cache();}
  if (has_category(options_.categories, CollectCategory::memory)){ info.memory_info = collect_memory();}
  if (has_category(options_.categories, CollectCategory::pci)){ info.pci_devices = collect_pci();}
  if (has_category(options_.categories, CollectCategory::gpu)){ info.gpu_list = collect_gpu();}
  if (has_category(options_.categories, CollectCategory::perf)){ info.perf_info = collect_perf();}
  if (has_category(options_.categories, CollectCategory::block)){ info.block_devices = collect_block();}
  if (has_category(options_.categories, CollectCategory::net)){ info.net_interfaces = collect_net();}
  if (has_category(options_.categories, CollectCategory::thermal)){ info.thermal_info = collect_thermal();}
  if (has_category(options_.categories, CollectCategory::power)){ info.power_info = collect_power();}
  if (has_category(options_.categories, CollectCategory::virtualization)){ info.virtualization_info = collect_virtualization();}
  if (has_category(options_.categories, CollectCategory::platform)){ info.platform_info = collect_platform();}
  
  return info;
}

OsInfo Collector::collect_os() const {
  OsInfo info;

  struct utsname uts {};
  if (uname(&uts) == 0) {
    info.kernel_name = StringField::value(uts.sysname);
    info.kernel_release = StringField::value(uts.release);
    info.kernel_version = StringField::value(uts.version);
    info.machine = StringField::value(uts.machine);
    info.hostname = StringField::value(uts.nodename);
  } else {
    info.kernel_name = StringField::unavailable(Status::internal_error);
    info.kernel_release = StringField::unavailable(Status::internal_error);
    info.kernel_version = StringField::unavailable(Status::internal_error);
    info.machine = StringField::unavailable(Status::internal_error);
    info.hostname = StringField::unavailable(Status::internal_error);
  }

  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    info.page_size = U64Field::value(static_cast<std::uint64_t>(page));
  } else {
    info.page_size = U64Field::unavailable(Status::not_found);
  }

  info.word_size = U64Field::value(sizeof(void*) * 8);

  std::uint16_t test = 1;
  bool little = *reinterpret_cast<unsigned char*>(&test) == 1;
  info.endianness = StringField::value(little ? "little" : "big");

  info.cmdline = StringField::value(read_first_line("/proc/cmdline"));

  // Minimal /etc/os-release parsing can come later.
  info.distro_name = StringField::unavailable(Status::unsupported);
  info.distro_version = StringField::unavailable(Status::unsupported);
  info.distro_id = StringField::unavailable(Status::unsupported);

  return info;
}

CpuInfo Collector::collect_cpu() const {
  CpuInfo info;
  info.arch = ArchType::unknown;

  struct utsname uts {};
  if (uname(&uts) == 0) {
    std::string machine = uts.machine;
    info.arch_name = StringField::value(machine);
    info.arch = arch_from_machine(machine);
  } else {
    info.arch_name = StringField::unavailable(Status::internal_error);
  }

  std::string present = read_first_line("/sys/devices/system/cpu/present");
  if (!present.empty()) {
    info.present_cpu_count = U64Field::value(count_cpu_range_list(present));
  } else {
    info.present_cpu_count = U64Field::unavailable(Status::not_found);
  }

  std::string online = read_first_line("/sys/devices/system/cpu/online");
  if (!online.empty()) {
    info.online_cpu_count = U64Field::value(count_cpu_range_list(online));
  } else {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n > 0) {
      info.online_cpu_count = U64Field::value(static_cast<std::uint64_t>(n));
    } else {
      info.online_cpu_count = U64Field::unavailable(Status::not_found);
    }
  }

  std::string possible = read_first_line("/sys/devices/system/cpu/possible");
  if (!possible.empty()) {
    info.possible_cpu_count = U64Field::value(count_cpu_range_list(possible));
  } else {
    info.possible_cpu_count = U64Field::unavailable(Status::not_found);
  }

  // TODO: parse /proc/cpuinfo for vendor, brand, family, model, stepping, microcode.
  info.vendor = StringField::unavailable(Status::unsupported);
  info.brand = StringField::unavailable(Status::unsupported);
  info.model_name = StringField::unavailable(Status::unsupported);

  info.sockets = U64Field::unavailable(Status::unsupported);
  info.cores_per_socket = U64Field::unavailable(Status::unsupported);
  info.threads_per_core = U64Field::unavailable(Status::unsupported);

  info.family = U64Field::unavailable(Status::unsupported);
  info.model = U64Field::unavailable(Status::unsupported);
  info.stepping = U64Field::unavailable(Status::unsupported);
  info.microcode = StringField::unavailable(Status::unsupported);

  info.physical_address_bits = U64Field::unavailable(Status::unsupported);
  info.virtual_address_bits = U64Field::unavailable(Status::unsupported);

  info.is_virtualized = BoolField::unavailable(Status::unsupported);
  info.hypervisor_vendor = StringField::unavailable(Status::unsupported);

  return info;
}

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

IsaFeatures Collector::collect_isa() const {
  return {};
}

CacheList Collector::collect_cache() const {
  return {};
}

PciDeviceList Collector::collect_pci() const {
  return {};
}

GpuList Collector::collect_gpu() const {
  return {};
}

PerfCounterInfo Collector::collect_perf() const {
  return {};
}

BlockDeviceList Collector::collect_block() const {
  return {};
}

NetInterfaceList Collector::collect_net() const {
  return {};
}

ThermalInfo Collector::collect_thermal() const {
  return {};
}

PowerInfo Collector::collect_power() const {
  return {};
}

VirtualizationInfo Collector::collect_virtualization() const {
  return {};
}

PlatformInfo Collector::collect_platform() const {
  return {};
}
// Convenience functions for one-off collection
SystemInfo collect_system() {
  return Collector{}.collect();
}

SystemInfo collect_system(const CollectOptions& options) {
  return Collector{options}.collect();
}

OsInfo collect_os() {
  return Collector{}.collect_os();
}

CpuInfo collect_cpu() {
  return Collector{}.collect_cpu();
}

IsaFeatures collect_isa() {
  return Collector{}.collect_isa();
}

CacheList collect_cache() {
  return Collector{}.collect_cache();
}

MemoryInfo collect_memory() {
  return Collector{}.collect_memory();
}

PciDeviceList collect_pci() {
  return Collector{}.collect_pci();
}

GpuList collect_gpu() {
  return Collector{}.collect_gpu();
}

PerfCounterInfo collect_perf() {
  return Collector{}.collect_perf();
}

BlockDeviceList collect_block() {
  return Collector{}.collect_block();
}

NetInterfaceList collect_net() {
  return Collector{}.collect_net();
}

ThermalInfo collect_thermal() {
  return Collector{}.collect_thermal();
}

PowerInfo collect_power() {
  return Collector{}.collect_power();
}

VirtualizationInfo collect_virtualization() {
  return Collector{}.collect_virtualization();
}

PlatformInfo collect_platform() {
  return Collector{}.collect_platform();
}

// placeholder JSON functions

std::string to_json(const SystemInfo& info) {
  return "{}";
}

std::string to_json(const OsInfo& features) {
  return "{}";
}
std::string to_json(const CpuInfo& features) {
  return "{}";
}
std::string to_json(const IsaFeatures& features) {
  return "{}";
}
std::string to_json(const CacheList& features) {
  return "{}";
}
std::string to_json(const MemoryInfo& features) {
  return "{}";
}
std::string to_json(const PciDeviceList& features) {
  return "{}";
}
std::string to_json(const GpuList& features) {
  return "{}";
}
std::string to_json(const PerfCounterInfo& features) {
  return "{}";
}
std::string to_json(const BlockDeviceList& features) {
  return "{}";
}
std::string to_json(const NetInterfaceList& features) {
  return "{}";
}
std::string to_json(const PowerInfo& features) {
  return "{}";
}
std::string to_json(const VirtualizationInfo& features) {
  return "{}";
}
std::string to_json(const PlatformInfo& features) {
  return "{}";
}


}