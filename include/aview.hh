#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <utility> 
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
struct CpuTopologyEntry {
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

  U64Field present_cpu_count; // /sys/devices/system/cpu/present
  U64Field online_cpu_count; // /sys/devices/system/cpu/online
  U64Field possible_cpu_count; // /sys/devices/system/cpu/possible
  U64Field sockets;
  
  U64Field cores_per_socket;
  U64Field threads_per_core;

  U64Field family;
  U64Field model;
  U64Field stepping;
  StringField microcode;

  U64Field physical_address_bits;
  U64Field virtual_address_bits;

  BoolField is_virtualized;
  StringField hypervisor_vendor;

  std::vector<CpuTopologyEntry> topology;
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
  BoolField x86_pclmulqdq;

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
  BoolField arm_neon;
  BoolField arm_asimd;
  BoolField arm_sve;
  BoolField arm_sve2;
  BoolField arm_aes;
  BoolField arm_sha1;
  BoolField arm_sha2;
  BoolField arm_crc32;
  BoolField arm_pauth;
  // riscv
  BoolField riscv_i;
  BoolField riscv_m;
  BoolField riscv_a;
  BoolField riscv_f;
  BoolField riscv_d;
  BoolField riscv_c;
  BoolField riscv_v;
  BoolField riscv_h;

  StringField raw_flags;
};

// Cache info
enum class CacheType {
  unknown,
  data,
  instruction,
  unified
};

std::string to_string(CacheType type);

struct CacheInfo {
  U32Field cpu_id;

  U32Field level;
  CacheType type = CacheType::unknown;

  U64Field size; // in bytes
  U64Field line_size; // in bytes
  U64Field associativity; // number of ways
  U64Field sets;

  StringField shared_cpu_list;
};

struct CacheList {
  std::vector<CacheInfo> entries;
};

// Memory / NUMA info
struct NumaNodeInfo {
  U32Field node_id;
  U64Field total_kb;
  U64Field free_kb;
  StringField cpu_list;
};

struct MemoryInfo {
  U64Field total_kb;
  U64Field free_kb;
  U64Field available_kb;
  U64Field buffers_kb;
  U64Field cached_kb;

  U64Field swap_total_kb;
  U64Field swap_free_kb;

  U64Field page_size_bytes;
  U64Field hugepage_size_kb;
  U64Field hugepages_total;
  U64Field hugepages_free;

  BoolField transparent_hugepages_enabled;

  std::vector<NumaNodeInfo> numa_nodes;
};

// PCI info
struct PciDeviceInfo {
  StringField pci_address;

  StringField vendor_id;
  StringField device_id;
  StringField subsystem_vendor_id;
  StringField subsystem_device_id;
  StringField class_id;

  StringField driver;
  I64Field numa_node;
  StringField iommu_group;

  StringField resource_path;
};

struct PciDeviceList {
  std::vector<PciDeviceInfo> entries;
};

// GPU info

enum class GpuVendor {
  unknown,
  intel,
  amd,
  nvidia
};

std::string to_string(GpuVendor vendor);

struct GpuInfo {
  GpuVendor vendor = GpuVendor::unknown;
  
  StringField name;
  StringField drm_card;
  StringField render_node;
  StringField pci_address;

  StringField vendor_id;
  StringField device_id;
  StringField driver;

  U64Field vram_total_bytes;
  U64Field vram_used_bytes;

  U64Field temp_millidegree_c;
  U64Field power_mw;

  U64Field graphics_clock_mhz;
  U64Field memory_clock_mhz;

  BoolField cuda_available;
  StringField cuda_compute_capability;
};

struct GpuList {
  std::vector<GpuInfo> entries;
};

// Performance Counter info

struct PerfCounterAvailability {
  BoolField cpu_cycles;
  BoolField instructions;
  BoolField cache_references;
  BoolField cache_misses;
  BoolField branch_instructions;
  BoolField branch_misses;
  BoolField bus_cycles;
  BoolField ref_cpu_cycles;
};

struct PerfCounterInfo {
  U64Field perf_event_paranoid;

  PerfCounterAvailability available;

  BoolField rdtsc_available;
  BoolField rdtscp_available;
  BoolField invariant_tsc;
};
// Block devices

struct BlockDeviceInfo {
  StringField name;
  StringField model;
  StringField vendor;
  StringField path;

  BoolField rotational;
  U64Field size_bytes;
  U64Field logical_block_size;
  U64Field physical_block_size;

  StringField scheduler;
};

struct BlockDeviceList {
  std::vector<BlockDeviceInfo> entries;
};

// Network
struct NetInterfaceInfo {
  StringField name;
  StringField mac_address;
  StringField ipv4_addresses;
  StringField ipv6_addresses;

  U64Field mtu;
  U64Field speed_mbps;

  StringField duplex;
  BoolField carrier;
  StringField driver;
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
  BoolField on_battery;
  StringField battery_status;
  U64Field battery_capacity_percent;

  StringField cpu_governor;
  U64Field cpu_current_freq_mhz;
  U64Field cpu_min_freq_mhz;
  U64Field cpu_max_freq_mhz;
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
  PerfCounterInfo perf_info;
  BlockDeviceList block_devices;
  NetInterfaceList net_interfaces;
  ThermalInfo thermal_info;
  PowerInfo power_info;
  VirtualizationInfo virtualization_info;
  PlatformInfo platform_info;
};
// Collection categories
// Using bit flags to allow all categories to be collected in one variable i.e.
// os == 00000001, cpu == 00000010, isa == 00000100
// collect_categories = os | cpu | isa == 00000111
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
// override operators for bitwise operations to convert enum to uint64_t before performing op then casting back to the enum
inline CollectCategory operator|(CollectCategory a, CollectCategory b) {
  return static_cast<CollectCategory>(static_cast<std::uint64_t>(a) | static_cast<std::uint64_t>(b));
}

inline CollectCategory operator&(CollectCategory a, CollectCategory b) {
  return static_cast<CollectCategory>(static_cast<std::uint64_t>(a) & static_cast<std::uint64_t>(b));
}

inline bool has_category(CollectCategory flags, CollectCategory category) {
  return static_cast<std::uint64_t>(flags & category) != 0;
}

struct CollectOptions {
  CollectCategory categories = CollectCategory::all;

  //modify later, lib should be read only and safe by default
  bool allow_slow_probes = false;
  bool allow_vendor_libraries = false;
  bool allow_perf_open = false;
};
 
class Collector {
public:
  Collector() = default;
  explicit Collector(CollectOptions options);

  const CollectOptions& options() const;
  void set_options(const CollectOptions& options);

  SystemInfo collect() const;

  OsInfo collect_os() const;
  CpuInfo collect_cpu() const;
  IsaFeatures collect_isa() const;
  CacheList collect_cache() const;
  MemoryInfo collect_memory() const;
  PciDeviceList collect_pci() const;
  GpuList collect_gpu() const;
  PerfCounterInfo collect_perf() const;
  BlockDeviceList collect_block() const;
  NetInterfaceList collect_net() const;
  ThermalInfo collect_thermal() const;
  PowerInfo collect_power() const;
  VirtualizationInfo collect_virtualization() const;
  PlatformInfo collect_platform() const;
private:
  CollectOptions options_;
};

// convenience functions for one-off collection

SystemInfo collect_system();
SystemInfo collect_system(const CollectOptions& options);

OsInfo collect_os();
CpuInfo collect_cpu();
IsaFeatures collect_isa();
CacheList collect_cache();
MemoryInfo collect_memory();
PciDeviceList collect_pci();
GpuList collect_gpu();
PerfCounterInfo collect_perf();
BlockDeviceList collect_block();
NetInterfaceList collect_net();
ThermalInfo collect_thermal();
PowerInfo collect_power();
VirtualizationInfo collect_virtualization();
PlatformInfo collect_platform();

// Json helper functions
std::string to_json(const SystemInfo& info);
std::string to_json(const OsInfo& info);
std::string to_json(const CpuInfo& info);
std::string to_json(const IsaFeatures& features);
std::string to_json(const CacheList& cache_list);
std::string to_json(const MemoryInfo& memory_info);
std::string to_json(const PciDeviceList& pci_devices);
std::string to_json(const GpuList& gpu_list);
std::string to_json(const PerfCounterInfo& perf_info);
std::string to_json(const BlockDeviceList& block_devices);
std::string to_json(const NetInterfaceList& net_interfaces);
std::string to_json(const ThermalInfo& thermal_info);
std::string to_json(const PowerInfo& power_info);
std::string to_json(const VirtualizationInfo& virtualization_info);
std::string to_json(const PlatformInfo& platform_info);

bool available(Status status);

template <typename T>
T value_or(const Field<T>& field, const T& default_value) {
  return field.valid() ? field.value() : default_value;

}
