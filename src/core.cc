#include "aview.hh"

#include <iostream>

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

bool available(Status status) {
  return status == Status::ok;
}

SystemInfo collect_system() {
  return Collector{}.collect();
}

SystemInfo collect_system(const CollectOptions& options) {
  return Collector{options}.collect();
}

}

int main() {
  auto system_info = archspec::collect_system();
  std::cout << "Kernel: " << system_info.os_info.kernel_name.value_or("unknown") << " " 
            << system_info.os_info.kernel_version.value_or("unknown") << " "
            << system_info.os_info.kernel_release.value_or("unknown") << std::endl;
  return 0;
}