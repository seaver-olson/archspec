#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace archspec {
enum class Status {
  ok,
  unsupported,
  perm_denied,
  not_found,
  parse_error,
  invalid_arg,
  internal_error
};

std::string to_string(Status status);

template <typename T>
class Field {
public:
  Field() = default;

  static Field<T> value(T value) {
    Field<T> field;
    field.valid_ = true;
    field.status_ = Status::ok;
    field.value_ = std::move(value);
    return field;
  }

  static Field<T> unavailable(Status status) {
    Field<T> field;
    field.valid_ = false;
    field.status_ = status;
    return field;
  }

  bool valid() const { return valid_; }

  explicit operator bool() const { return valid_; }

  const T& value() const { return value_; }

  T& get() { return value_; }
  
  // Return the value if valid, otherwise return the provided default value.
  const T& value_or(const T& default_value) const {
    return valid_ ? value_ : default_value;
  }

  Status status() const { return status_; }
private:
  bool valid_ = false;
  Status status_ = Status::not_found;
  T value_{};
};

// short-hand fields
using BoolField = Field<bool>;
using U32Field = Field<std::uint32_t>;
using U64Field = Field<std::uint64_t>;
using I64Field = Field<std::int64_t>;
using StringField = Field<std::string>;


// Architecture

enum class ArchType{
  unknown,
  x86,
  x86_64,
  arm,
  aarch64,
  riscv32,
  riscv64,
  ppc64,
  s390x
};

std::string to_string(ArchType arch);

// OS info
struct OsInfo {
  StringField kernel_name;
  StringField kernel_version;
  StringField kernel_release;
  
  StringField machine; // x86_64, armv8l, etc.
  StringField hostname;

  StringField distro_name;
  StringField distro_version;
  StringField distro_id;

  StringField cmdline;

  U64Field page_size;
  U64Field word_size; // in bits
  StringField endianness; // little, big, undefined
};

// CPU info
struct CpuToplogyEntry {
  U32Field logical_id;
  U32Field core_id;
  U32Field package_id;
  U32Field numa_node;

  StringField thread_siblings_mask;
  StringField core_siblings_mask;
};

struct CpuInfo {
  ArchType arch;

  StringField arch_name;
  StringField vendor;
  StringField brand;
  StringField model_name;

  U64Field logical_cores;
};


// ISA Features

struct IsaFeatures {
   // x86/x86_64
  BoolField x86_fpu;
  BoolField x86_mmx;
  
  BoolField x86_sse;
  BoolField x86_sse2;
  BoolField x86_sse3;
  BoolField x86_ssse3;
  BoolField x86_sse41;
  BoolField x86_sse42;

  BoolField x86_avx;
  BoolField x86_avx2;
  BoolField x86_avx512f;
  BoolField x86_avx512dq;
  BoolField x86_avx512cd;
  BoolField x86_avx512bw;
  BoolField x86_avx512v1;

  BoolField x86_aes;
  BoolField x86_sha;
  BoolField x86_pc1mu1qdq;

  BoolField x86_rdrand;
  BoolField x86_rdseed;

  BoolField x86_xsave;
  BoolField x86_xsaveopt;
  BoolField x86_xsavec;

  // Intel VT-x
  BoolField x86_vmx;
  BoolField x86_ept;
  // AMD SVM
  BoolField x86_svm;
  BoolField x86_npt;
  
  BoolField x86_tsc;
  BoolField x86_constant_tsc;
  BoolField x86_nonstop_tsc;
  BoolField x86_rdtscp;

  // arm/aarch64

  // riscv

};

// Cache info

enum class CacheType {

};

std::string to_string(CacheType type);

struct CacheInfo {
  
};

struct CacheList {

};

// Memory / NUMA info

struct NumaNodeInfo {

};

struct MemoryInfo {
  
};

// PCI info

struct PciDeviceInfo {

};

struct PciDeviceList {

};

// GPU info

enum class GpuVendor {

};

std::string to_string(GpuVendor vendor);

struct GpuInfo {

};

struct GpuList {
  std::vector<GpuInfo> entries;
};

// Performance Counter info

struct PerfCounterAvailability {

};

struct PerfCounterInfo {

};
// Block devices

struct BlockDeviceInfo {

};

struct BlockDeviceList {
  std::vector<BlockDeviceInfo> entries;
};

// Network

struct NetInterfaceInfo {

};

struct NetInterfaceList {
  std::vector<NetInterfaceInfo> entries;
};

// Thermal / Power
struct ThermalZoneInfo {
  StringField name;
  StringField type;
  StringField temp; //in millidegree Celsius
};

struct ThermalInfo {
  std::vector<ThermalZoneInfo> entries;
};

struct PowerInfo {

};


// Virtualization / platform

struct VirtualizationInfo {
  BoolField running_under_hypervisor;
  StringField hypervisor_vendor;

  BoolField kvm_available;
  BoolField dev_kvm_exists;

  BoolField intel_vmx_available;
  BoolField amd_svm_available;

  BoolField iommu_present;
};

struct PlatformInfo {
  StringField system_vendor;
  StringField product_name;
  StringField product_version;

  StringField board_vendor;
  StringField board_name;

  StringField bios_vendor;
  StringField bios_version;
  StringField bios_date;

  BoolField uefi;
  BoolField secure_boot;
};

// Full system info

struct SystemInfo {
  OsInfo os_info;
  CpuInfo cpu_info;
  IsaFeatures isa_features;
  CacheList cache_list;
  MemoryInfo memory_info;
  PciDeviceList pci_devices;
  GpuList gpu_list;
  PerfCounterAvailability perf_counters;
  BlockDeviceList block_devices;
  NetInterfaceList net_interfaces;
  ThermalInfo thermal_info;
  PowerInfo power_info;
  VirtualizationInfo virtualization_info;
  PlatformInfo platform_info;
};
// Collection categories
//
enum class CollectCategory : std::uint64_t {
    none           = 0,

    os             = 1ull << 0,
    cpu            = 1ull << 1,
    isa            = 1ull << 2,
    cache          = 1ull << 3,
    memory         = 1ull << 4,
    pci            = 1ull << 5,
    gpu            = 1ull << 6,
    perf           = 1ull << 7,
    block          = 1ull << 8,
    net            = 1ull << 9,
    thermal        = 1ull << 10,
    power          = 1ull << 11,
    virtualization = 1ull << 12,
    platform       = 1ull << 13,

    all            = ~0ull
};

}
