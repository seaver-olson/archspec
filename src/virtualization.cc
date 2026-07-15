#include "aview.hh"
#include "internal.hh"

#include <algorithm>
#include <set>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace archspec {
namespace {

std::string lower_copy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool contains_virtualization_hint(const std::string& value) {
  std::string lower = lower_copy(value);
  for (const char* hint : {
           "kvm", "qemu", "vmware", "virtualbox", "hyper-v", "hyperv",
           "xen", "bochs", "parallels", "bhyve", "wsl",
       }) {
    if (lower.find(hint) != std::string::npos) {
      return true;
    }
  }

  return false;
}

struct CpuFlags {
  Status status = Status::not_found;
  std::set<std::string> values;
  bool x86 = false;
};

CpuFlags cpu_flags(const CollectOptions& options) {
  detail::ReadResult cpuinfo = detail::read_file(detail::proc_path(options, "/cpuinfo"));
  if (cpuinfo.status != Status::ok) {
    return {cpuinfo.status, {}, false};
  }

  std::vector<detail::KeyValueMap> blocks = detail::parse_colon_blocks(cpuinfo.value);
  if (blocks.empty()) {
    return {Status::parse_error, {}, false};
  }

  for (const char* key : {"flags", "Features"}) {
    auto value = detail::map_value(blocks.front(), key);
    if (value) {
      return {Status::ok, detail::word_set(*value), std::string(key) == "flags"};
    }
  }

  return {Status::not_found, {}, false};
}

StringField detected_hypervisor_vendor(const CollectOptions& options) {
  for (const std::string& path : {
           detail::sys_path(options, "/hypervisor/type"),
           detail::sys_path(options, "/class/dmi/id/product_name"),
           detail::sys_path(options, "/class/dmi/id/sys_vendor"),
       }) {
    StringField value = detail::read_string_field(path);
    if (value.valid() && contains_virtualization_hint(value.value())) {
      return value;
    }
  }

  return StringField::unavailable(Status::not_found);
}

BoolField iommu_present(const CollectOptions& options) {
  std::string root = detail::sys_path(options, "/kernel/iommu_groups");
  if (!detail::path_is_dir(root)) {
    return BoolField::unavailable(Status::not_found);
  }

  for (const std::string& name : detail::list_dir(root)) {
    std::uint64_t id = 0;
    if (detail::parse_u64(name, id)) {
      return BoolField::value(true);
    }
  }

  return BoolField::value(false);
}

} // namespace

VirtualizationInfo Collector::collect_virtualization() const {
  VirtualizationInfo info;
  CpuFlags flags = cpu_flags(options_);

  bool cpu_hypervisor = flags.values.find("hypervisor") != flags.values.end();
  StringField vendor = detected_hypervisor_vendor(options_);
  if (flags.status == Status::ok || vendor.valid()) {
    info.running_under_hypervisor = BoolField::value(cpu_hypervisor || vendor.valid());
  } else {
    info.running_under_hypervisor = BoolField::unavailable(flags.status);
  }
  info.hypervisor_vendor = vendor;

  std::string kvm = detail::dev_path(options_, "/kvm");
  bool kvm_exists = detail::path_exists(kvm);
  info.dev_kvm_exists = BoolField::value(kvm_exists);
#if defined(_WIN32)
  info.kvm_available = BoolField::unavailable(Status::unsupported);
#else
  info.kvm_available = BoolField::value(kvm_exists && access(kvm.c_str(), R_OK | W_OK) == 0);
#endif

  if (flags.status == Status::ok && flags.x86) {
    info.intel_vmx_available = detail::bool_from_flag(flags.values, "vmx");
    info.amd_svm_available = detail::bool_from_flag(flags.values, "svm");
  } else if (flags.status == Status::ok) {
    info.intel_vmx_available = BoolField::unavailable(Status::unsupported);
    info.amd_svm_available = BoolField::unavailable(Status::unsupported);
  } else {
    info.intel_vmx_available = BoolField::unavailable(flags.status);
    info.amd_svm_available = BoolField::unavailable(flags.status);
  }
  info.iommu_present = iommu_present(options_);

  return info;
}

VirtualizationInfo collect_virtualization() { return Collector{}.collect_virtualization(); }

} // namespace archspec
