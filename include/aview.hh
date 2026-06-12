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

};

// CPU info
struct CpuToplogyEntry {

};

struct CpuInfo {

};


// ISA Features

struct IsaFeatures {

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

};

// Network

struct NetInterfaceInfo {

};

struct NetInterfaceList {

};

// Thermal / Power

struct ThermalZoneInfo {

};

struct ThermalInfo {

};

struct PowerInfo {

};


// Virtualization / platform

struct VirtualizationInfo {

};

struct PlatformInfo {

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
