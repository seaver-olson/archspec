#include "aview.hh"

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace archspec {

namespace {

std::string escape_json(const std::string& input) {
  std::ostringstream out;

  for (unsigned char c : input) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u"
              << std::hex
              << std::setw(4)
              << std::setfill('0')
              << static_cast<int>(c)
              << std::dec;
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }

  return out.str();
}

std::string json_string(const std::string& value) {
  return "\"" + escape_json(value) + "\"";
}

std::string json_bool(bool value) {
  return value ? "true" : "false";
}

class JsonObject {
public:
  void add_raw(const std::string& key, const std::string& raw_value){
    add_comma();
    out_ << json_string(key) << ":" << raw_value;
  }

  void add_string(const std::string& key, const std::string& value){
    add_raw(key, json_string(value));
  }

  void add_u64(const std::string& key, const std::uint64_t value){
    add_comma();
    out_ << json_string(key) << ":" << value;
  }

  void add_i64(const std::string& key, const std::int64_t value){
    add_comma();
    out_ << json_string(key) << ":" << value;
  }

  void add_bool(const std::string& key, const bool value){
    add_raw(key, json_bool(value));
  }

  std::string str() const {
    return "{" + out_.str() + "}";
  }
private:
  void add_comma() {
    if (!first_) {
      out_ << ",";
    } else {
      first_ = false;
    }
  }
  bool first_ = true;
  std::ostringstream out_;
};

std::string field_to_json(const StringField& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));
  if (field.valid()) {
    obj.add_string("value", field.value());
  }
  return obj.str();
}

std::string field_to_json(const BoolField& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_bool("value", field.value());
  }

  return obj.str();
}

std::string field_to_json(const U32Field& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_u64("value", field.value());
  }

  return obj.str();
}

std::string field_to_json(const U64Field& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_u64("value", field.value());
  }

  return obj.str();
}

std::string field_to_json(const I64Field& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_i64("value", field.value());
  }

  return obj.str();
}

void add_field(JsonObject& obj, const std::string& key, const StringField& field) {
  obj.add_raw(key, field_to_json(field));
}

void add_field(JsonObject& obj, const std::string& key, const BoolField& field) {
  obj.add_raw(key, field_to_json(field));
}

void add_field(JsonObject& obj, const std::string& key, const U32Field& field) {
  obj.add_raw(key, field_to_json(field));
}

void add_field(JsonObject& obj, const std::string& key, const U64Field& field) {
  obj.add_raw(key, field_to_json(field));
}

void add_field(JsonObject& obj, const std::string& key, const I64Field& field) {
  obj.add_raw(key, field_to_json(field));
}

//converts std::vector<T> to JSON array (for lists of components)
template <typename T, typename Fn>
std::string vector_to_json(const std::vector<T>& values, Fn to_json_fn){
  std::ostringstream out;
  out << "[";

  bool first = true;
  for (const auto& value : values) {
    if (!first) {
      out << ",";
    }

    out << to_json_fn(value);
    first = false;
  }

  out << "]";

  return out.str();
}

std::string cpu_topology_to_json(const CpuTopologyEntry& entry) {
  JsonObject obj;

  add_field(obj, "logical_id", entry.logical_id);
  add_field(obj, "core_id", entry.core_id);
  add_field(obj, "package_id", entry.package_id);
  add_field(obj, "numa_node", entry.numa_node);
  add_field(obj, "thread_siblings_mask", entry.thread_siblings_mask);
  add_field(obj, "core_siblings_mask", entry.core_siblings_mask);

  return obj.str();
}

std::string cache_info_to_json(const CacheInfo& cache){
  JsonObject obj;

  add_field(obj, "cpu_id", cache.cpu_id);
  add_field(obj, "level", cache.level);
  obj.add_string("type", to_string(cache.type));

  add_field(obj, "size", cache.size);
  add_field(obj, "line_size", cache.line_size);
  add_field(obj, "associativity", cache.associativity);
  add_field(obj, "sets", cache.sets);
  add_field(obj, "shared_cpu_list", cache.shared_cpu_list);
  
  return obj.str();
}

std::string numa_node_to_json(const NumaNodeInfo& node) {
  JsonObject obj;

  add_field(obj, "node_id", node.node_id);
  add_field(obj, "total_kb", node.total_kb);
  add_field(obj, "free_kb", node.free_kb);
  add_field(obj, "cpu_list", node.cpu_list);

  return obj.str();
}

std::string pci_device_to_json(const PciDeviceInfo& device) {
  JsonObject obj;

  add_field(obj, "pci_address", device.pci_address);
  add_field(obj, "vendor_id", device.vendor_id);
  add_field(obj, "device_id", device.device_id);
  add_field(obj, "subsystem_vendor_id", device.subsystem_vendor_id);
  add_field(obj, "subsystem_device_id", device.subsystem_device_id);
  add_field(obj, "class_id", device.class_id);
  add_field(obj, "driver", device.driver);
  add_field(obj, "numa_node", device.numa_node);
  add_field(obj, "iommu_group", device.iommu_group);
  add_field(obj, "resource_path", device.resource_path);

  return obj.str();
}

std::string gpu_info_to_json(const GpuInfo& gpu) {
  JsonObject obj;

  obj.add_string("vendor", to_string(gpu.vendor));

  add_field(obj, "name", gpu.name);
  add_field(obj, "drm_card", gpu.drm_card);
  add_field(obj, "render_node", gpu.render_node);
  add_field(obj, "pci_address", gpu.pci_address);
  add_field(obj, "vendor_id", gpu.vendor_id);
  add_field(obj, "device_id", gpu.device_id);
  add_field(obj, "driver", gpu.driver);

  add_field(obj, "vram_total_bytes", gpu.vram_total_bytes);
  add_field(obj, "vram_used_bytes", gpu.vram_used_bytes);

  add_field(obj, "temp_millidegree_c", gpu.temp_millidegree_c);
  add_field(obj, "power_mw", gpu.power_mw);

  add_field(obj, "graphics_clock_mhz", gpu.graphics_clock_mhz);
  add_field(obj, "memory_clock_mhz", gpu.memory_clock_mhz);

  add_field(obj, "cuda_available", gpu.cuda_available);
  add_field(obj, "cuda_compute_capability", gpu.cuda_compute_capability);

  return obj.str();
}

std::string block_device_to_json(const BlockDeviceInfo& block) {
  JsonObject obj;

  add_field(obj, "name", block.name);
  add_field(obj, "model", block.model);
  add_field(obj, "vendor", block.vendor);
  add_field(obj, "path", block.path);
  add_field(obj, "rotational", block.rotational);
  add_field(obj, "size_bytes", block.size_bytes);
  add_field(obj, "logical_block_size", block.logical_block_size);
  add_field(obj, "physical_block_size", block.physical_block_size);
  add_field(obj, "scheduler", block.scheduler);

  return obj.str();
}

std::string net_interface_to_json(const NetInterfaceInfo& net) {
  JsonObject obj;

  add_field(obj, "name", net.name);
  add_field(obj, "mac_address", net.mac_address);
  add_field(obj, "ipv4_addresses", net.ipv4_addresses);
  add_field(obj, "ipv6_addresses", net.ipv6_addresses);
  add_field(obj, "mtu", net.mtu);
  add_field(obj, "speed_mbps", net.speed_mbps);
  add_field(obj, "duplex", net.duplex);
  add_field(obj, "carrier", net.carrier);
  add_field(obj, "driver", net.driver);

  return obj.str();
}

std::string thermal_zone_to_json(const ThermalZoneInfo& zone) {
  JsonObject obj;

  add_field(obj, "name", zone.name);
  add_field(obj, "type", zone.type);
  add_field(obj, "temp", zone.temp);

  return obj.str();
}

} // anonymous namespace

std::string to_json(const SystemInfo& info) {
  JsonObject obj;

  obj.add_raw("os_info", to_json(info.os_info));
  obj.add_raw("cpu_info", to_json(info.cpu_info));
  obj.add_raw("isa_features", to_json(info.isa_features));
  obj.add_raw("cache_list", to_json(info.cache_list));
  obj.add_raw("memory_info", to_json(info.memory_info));
  obj.add_raw("pci_devices", to_json(info.pci_devices));
  obj.add_raw("gpu_list", to_json(info.gpu_list));
  obj.add_raw("perf_info", to_json(info.perf_info));
  obj.add_raw("block_devices", to_json(info.block_devices));
  obj.add_raw("net_interfaces", to_json(info.net_interfaces));
  obj.add_raw("thermal_info", to_json(info.thermal_info));
  obj.add_raw("power_info", to_json(info.power_info));
  obj.add_raw("virtualization_info", to_json(info.virtualization_info));
  obj.add_raw("platform_info", to_json(info.platform_info));

  return obj.str();
}

std::string to_json(const OsInfo& info) {
  JsonObject obj;

  add_field(obj, "kernel_name", info.kernel_name);
  add_field(obj, "kernel_version", info.kernel_version);
  add_field(obj, "kernel_release", info.kernel_release);
  add_field(obj, "machine", info.machine);
  add_field(obj, "hostname", info.hostname);

  add_field(obj, "distro_name", info.distro_name);
  add_field(obj, "distro_version", info.distro_version);
  add_field(obj, "distro_id", info.distro_id);

  add_field(obj, "cmdline", info.cmdline);

  add_field(obj, "page_size", info.page_size);
  add_field(obj, "word_size", info.word_size);
  add_field(obj, "endianness", info.endianness);

  return obj.str();
}

std::string to_json(const CpuInfo& info) {
  JsonObject obj;

  obj.add_string("arch", to_string(info.arch));

  add_field(obj, "arch_name", info.arch_name);
  add_field(obj, "vendor", info.vendor);
  add_field(obj, "brand", info.brand);
  add_field(obj, "model_name", info.model_name);

  add_field(obj, "present_cpu_count", info.present_cpu_count);
  add_field(obj, "online_cpu_count", info.online_cpu_count);
  add_field(obj, "possible_cpu_count", info.possible_cpu_count);
  add_field(obj, "sockets", info.sockets);

  add_field(obj, "cores_per_socket", info.cores_per_socket);
  add_field(obj, "threads_per_core", info.threads_per_core);

  add_field(obj, "family", info.family);
  add_field(obj, "model", info.model);
  add_field(obj, "stepping", info.stepping);
  add_field(obj, "microcode", info.microcode);

  add_field(obj, "physical_address_bits", info.physical_address_bits);
  add_field(obj, "virtual_address_bits", info.virtual_address_bits);

  add_field(obj, "is_virtualized", info.is_virtualized);
  add_field(obj, "hypervisor_vendor", info.hypervisor_vendor);

  obj.add_raw(
      "topology",
      vector_to_json(info.topology, cpu_topology_to_json)
  );

  return obj.str();
}

std::string to_json(const IsaFeatures& features) {
  JsonObject obj;

  add_field(obj, "x86_fpu", features.x86_fpu);
  add_field(obj, "x86_mmx", features.x86_mmx);

  add_field(obj, "x86_sse", features.x86_sse);
  add_field(obj, "x86_sse2", features.x86_sse2);
  add_field(obj, "x86_sse3", features.x86_sse3);
  add_field(obj, "x86_ssse3", features.x86_ssse3);
  add_field(obj, "x86_sse41", features.x86_sse41);
  add_field(obj, "x86_sse42", features.x86_sse42);

  add_field(obj, "x86_avx", features.x86_avx);
  add_field(obj, "x86_avx2", features.x86_avx2);
  add_field(obj, "x86_avx512f", features.x86_avx512f);
  add_field(obj, "x86_avx512dq", features.x86_avx512dq);
  add_field(obj, "x86_avx512cd", features.x86_avx512cd);
  add_field(obj, "x86_avx512bw", features.x86_avx512bw);
  add_field(obj, "x86_avx512vl", features.x86_avx512vl);

  add_field(obj, "x86_aes", features.x86_aes);
  add_field(obj, "x86_sha", features.x86_sha);
  add_field(obj, "x86_pclmulqdq", features.x86_pclmulqdq);

  add_field(obj, "x86_rdrand", features.x86_rdrand);
  add_field(obj, "x86_rdseed", features.x86_rdseed);

  add_field(obj, "x86_xsave", features.x86_xsave);
  add_field(obj, "x86_xsaveopt", features.x86_xsaveopt);
  add_field(obj, "x86_xsavec", features.x86_xsavec);

  add_field(obj, "x86_vmx", features.x86_vmx);
  add_field(obj, "x86_ept", features.x86_ept);
  add_field(obj, "x86_svm", features.x86_svm);
  add_field(obj, "x86_npt", features.x86_npt);

  add_field(obj, "x86_tsc", features.x86_tsc);
  add_field(obj, "x86_constant_tsc", features.x86_constant_tsc);
  add_field(obj, "x86_nonstop_tsc", features.x86_nonstop_tsc);
  add_field(obj, "x86_rdtscp", features.x86_rdtscp);

  add_field(obj, "arm_neon", features.arm_neon);
  add_field(obj, "arm_asimd", features.arm_asimd);
  add_field(obj, "arm_sve", features.arm_sve);
  add_field(obj, "arm_sve2", features.arm_sve2);
  add_field(obj, "arm_aes", features.arm_aes);
  add_field(obj, "arm_sha1", features.arm_sha1);
  add_field(obj, "arm_sha2", features.arm_sha2);
  add_field(obj, "arm_crc32", features.arm_crc32);
  add_field(obj, "arm_pauth", features.arm_pauth);

  add_field(obj, "riscv_i", features.riscv_i);
  add_field(obj, "riscv_m", features.riscv_m);
  add_field(obj, "riscv_a", features.riscv_a);
  add_field(obj, "riscv_f", features.riscv_f);
  add_field(obj, "riscv_d", features.riscv_d);
  add_field(obj, "riscv_c", features.riscv_c);
  add_field(obj, "riscv_v", features.riscv_v);
  add_field(obj, "riscv_h", features.riscv_h);

  add_field(obj, "raw_flags", features.raw_flags);

  return obj.str();
}

std::string to_json(const CacheList& cache_list) {
  JsonObject obj;

  obj.add_raw(
      "entries",
      vector_to_json(cache_list.entries, cache_info_to_json)
  );

  return obj.str();
}

std::string to_json(const MemoryInfo& memory_info) {
  JsonObject obj;

  add_field(obj, "total_kb", memory_info.total_kb);
  add_field(obj, "free_kb", memory_info.free_kb);
  add_field(obj, "available_kb", memory_info.available_kb);
  add_field(obj, "buffers_kb", memory_info.buffers_kb);
  add_field(obj, "cached_kb", memory_info.cached_kb);

  add_field(obj, "swap_total_kb", memory_info.swap_total_kb);
  add_field(obj, "swap_free_kb", memory_info.swap_free_kb);

  add_field(obj, "page_size_bytes", memory_info.page_size_bytes);
  add_field(obj, "hugepage_size_kb", memory_info.hugepage_size_kb);
  add_field(obj, "hugepages_total", memory_info.hugepages_total);
  add_field(obj, "hugepages_free", memory_info.hugepages_free);

  add_field(
      obj,
      "transparent_hugepages_enabled",
      memory_info.transparent_hugepages_enabled
  );

  obj.add_raw(
      "numa_nodes",
      vector_to_json(memory_info.numa_nodes, numa_node_to_json)
  );

  return obj.str();
}

std::string to_json(const PciDeviceList& pci_devices) {
  JsonObject obj;

  obj.add_raw(
      "entries",
      vector_to_json(pci_devices.entries, pci_device_to_json)
  );

  return obj.str();
}

std::string to_json(const GpuList& gpu_list) {
  JsonObject obj;

  obj.add_raw(
      "entries",
      vector_to_json(gpu_list.entries, gpu_info_to_json)
  );

  return obj.str();
}

std::string to_json(const PerfCounterInfo& perf_info) {
  JsonObject obj;

  add_field(obj, "perf_event_paranoid", perf_info.perf_event_paranoid);

  JsonObject available;
  add_field(available, "cpu_cycles", perf_info.available.cpu_cycles);
  add_field(available, "instructions", perf_info.available.instructions);
  add_field(available, "cache_references", perf_info.available.cache_references);
  add_field(available, "cache_misses", perf_info.available.cache_misses);
  add_field(available, "branch_instructions", perf_info.available.branch_instructions);
  add_field(available, "branch_misses", perf_info.available.branch_misses);
  add_field(available, "bus_cycles", perf_info.available.bus_cycles);
  add_field(available, "ref_cpu_cycles", perf_info.available.ref_cpu_cycles);

  obj.add_raw("available", available.str());

  add_field(obj, "rdtsc_available", perf_info.rdtsc_available);
  add_field(obj, "rdtscp_available", perf_info.rdtscp_available);
  add_field(obj, "invariant_tsc", perf_info.invariant_tsc);

  return obj.str();
}

std::string to_json(const BlockDeviceList& block_devices) {
  JsonObject obj;

  obj.add_raw(
      "entries",
      vector_to_json(block_devices.entries, block_device_to_json)
  );

  return obj.str();
}

std::string to_json(const NetInterfaceList& net_interfaces) {
  JsonObject obj;

  obj.add_raw(
      "entries",
      vector_to_json(net_interfaces.entries, net_interface_to_json)
  );

  return obj.str();
}

std::string to_json(const ThermalInfo& thermal_info) {
  JsonObject obj;

  obj.add_raw(
      "entries",
      vector_to_json(thermal_info.entries, thermal_zone_to_json)
  );

  return obj.str();
}

std::string to_json(const PowerInfo& power_info) {
  JsonObject obj;

  add_field(obj, "on_battery", power_info.on_battery);
  add_field(obj, "battery_status", power_info.battery_status);
  add_field(obj, "battery_capacity_percent", power_info.battery_capacity_percent);

  add_field(obj, "cpu_governor", power_info.cpu_governor);
  add_field(obj, "cpu_current_freq_mhz", power_info.cpu_current_freq_mhz);
  add_field(obj, "cpu_min_freq_mhz", power_info.cpu_min_freq_mhz);
  add_field(obj, "cpu_max_freq_mhz", power_info.cpu_max_freq_mhz);

  return obj.str();
}

std::string to_json(const VirtualizationInfo& virtualization_info) {
  JsonObject obj;

  add_field(
      obj,
      "running_under_hypervisor",
      virtualization_info.running_under_hypervisor
  );

  add_field(obj, "hypervisor_vendor", virtualization_info.hypervisor_vendor);

  add_field(obj, "kvm_available", virtualization_info.kvm_available);
  add_field(obj, "dev_kvm_exists", virtualization_info.dev_kvm_exists);

  add_field(obj, "intel_vmx_available", virtualization_info.intel_vmx_available);
  add_field(obj, "amd_svm_available", virtualization_info.amd_svm_available);

  add_field(obj, "iommu_present", virtualization_info.iommu_present);

  return obj.str();
}

std::string to_json(const PlatformInfo& platform_info) {
  JsonObject obj;

  add_field(obj, "system_vendor", platform_info.system_vendor);
  add_field(obj, "product_name", platform_info.product_name);
  add_field(obj, "product_version", platform_info.product_version);

  add_field(obj, "board_vendor", platform_info.board_vendor);
  add_field(obj, "board_name", platform_info.board_name);

  add_field(obj, "bios_vendor", platform_info.bios_vendor);
  add_field(obj, "bios_version", platform_info.bios_version);
  add_field(obj, "bios_date", platform_info.bios_date);

  add_field(obj, "uefi", platform_info.uefi);
  add_field(obj, "secure_boot", platform_info.secure_boot);

  return obj.str();
}

} // namespace archspec
