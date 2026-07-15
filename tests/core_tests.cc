#include <archspec/archspec.hpp>

#include "../src/internal.hh"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

void write_file(
    const std::filesystem::path& root,
    const std::string& relative_path,
    const std::string& content
) {
  const std::filesystem::path path = root / relative_path;
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  require(static_cast<bool>(out), "fixture file can be created");
  out << content;
}

void make_fixture(const std::filesystem::path& root) {
  write_file(root, "proc/cpuinfo",
      "processor : 0\n"
      "vendor_id : GenuineIntel\n"
      "model name : Fixture CPU\n"
      "cpu family : 6\n"
      "model : 42\n"
      "stepping : 7\n"
      "microcode : 0x1\n"
      "address sizes : 46 bits physical, 57 bits virtual\n"
      "physical id : 0\n"
      "core id : 0\n"
      "cpu cores : 1\n"
      "siblings : 2\n"
      "flags : fpu sse2 aes avx2 tsc constant_tsc rdtscp hypervisor vmx\n\n"
      "processor : 1\n"
      "physical id : 0\n"
      "core id : 0\n"
  );
  write_file(root, "proc/meminfo",
      "MemTotal: 1024 kB\nMemFree: 128 kB\nMemAvailable: 512 kB\n"
      "Buffers: 8 kB\nCached: 64 kB\nSwapTotal: 32 kB\nSwapFree: 16 kB\n"
      "Hugepagesize: 2048 kB\nHugePages_Total: 2\nHugePages_Free: 1\n");
  write_file(root, "proc/sys/kernel/perf_event_paranoid", "2\n");
  write_file(root, "proc/cmdline", "private=fixture\n");
  write_file(root, "etc/os-release", "NAME=FixtureOS\nID=fixture\nVERSION_ID=1\n");

  write_file(root, "sys/devices/system/cpu/present", "0-1\n");
  write_file(root, "sys/devices/system/cpu/online", "0-1\n");
  write_file(root, "sys/devices/system/cpu/possible", "0-1\n");
  for (const char* cpu : {"cpu0", "cpu1"}) {
    const std::string base = std::string("sys/devices/system/cpu/") + cpu;
    write_file(root, base + "/topology/core_id", "0\n");
    write_file(root, base + "/topology/physical_package_id", "0\n");
    write_file(root, base + "/topology/thread_siblings", "00000003\n");
    write_file(root, base + "/topology/core_siblings", "00000003\n");
    write_file(root, base + "/cache/index0/level", "1\n");
    write_file(root, base + "/cache/index0/type", "Data\n");
    write_file(root, base + "/cache/index0/size", "32K\n");
    write_file(root, base + "/cache/index0/coherency_line_size", "64\n");
    write_file(root, base + "/cache/index0/ways_of_associativity", "8\n");
    write_file(root, base + "/cache/index0/number_of_sets", "64\n");
    write_file(root, base + "/cache/index0/shared_cpu_list", "0-1\n");
  }
  write_file(root, "sys/devices/system/node/node0/cpulist", "0-1\n");
  write_file(root, "sys/devices/system/node/node0/meminfo",
      "Node 0 MemTotal: 1024 kB\nNode 0 MemFree: 128 kB\n");
  write_file(root, "sys/kernel/mm/transparent_hugepage/enabled", "always [madvise] never\n");
  write_file(root, "sys/kernel/iommu_groups/0/placeholder", "\n");
  write_file(root, "sys/hypervisor/type", "KVM\n");

  const std::string pci = "sys/bus/pci/devices/0000:00:02.0/";
  write_file(root, pci + "vendor", "0x8086\n");
  write_file(root, pci + "device", "0x1234\n");
  write_file(root, pci + "class", "0x030000\n");
  write_file(root, pci + "numa_node", "0\n");
  write_file(root, pci + "resource", "\n");
  write_file(root, pci + "mem_info_vram_total", "bad\n");
  write_file(root, pci + "hwmon/hwmon0/temp1_input", "42000\n");
  write_file(root, pci + "hwmon/hwmon0/power1_average", "1234000\n");

  write_file(root, "sys/block/sda/size", "2\n");
  write_file(root, "sys/block/sda/device/model", "FixtureDisk\n");
  write_file(root, "sys/block/sda/queue/rotational", "0\n");
  write_file(root, "sys/block/sda/queue/logical_block_size", "512\n");
  write_file(root, "sys/block/sda/queue/physical_block_size", "4096\n");
  write_file(root, "sys/block/sda/queue/scheduler", "[none] mq-deadline\n");

  write_file(root, "sys/class/net/eth0/address", "02:00:00:00:00:01\n");
  write_file(root, "sys/class/net/eth0/mtu", "1500\n");
  write_file(root, "sys/class/net/eth0/speed", "-1\n");
  write_file(root, "sys/class/net/eth0/duplex", "full\n");
  write_file(root, "sys/class/net/eth0/carrier", "1\n");

  write_file(root, "sys/class/thermal/thermal_zone0/type", "x86_pkg_temp\n");
  write_file(root, "sys/class/thermal/thermal_zone0/temp", "42000\n");
  write_file(root, "sys/class/power_supply/BAT0/type", "Battery\n");
  write_file(root, "sys/class/power_supply/BAT0/status", "Discharging\n");
  write_file(root, "sys/class/power_supply/BAT0/capacity", "73\n");
  write_file(root, "sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "performance\n");
  write_file(root, "sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "bad\n");

  const std::string dmi = "sys/class/dmi/id/";
  write_file(root, dmi + "sys_vendor", "Fixture Corp\n");
  write_file(root, dmi + "product_name", "Fixture Machine\n");
  write_file(root, dmi + "board_name", "Fixture Board\n");
}

} // namespace

int main() {
  require(archspec::status_code(archspec::Status::perm_denied) == "perm_denied",
          "status codes are stable machine identifiers");
  require(archspec::to_string(archspec::Status::perm_denied) == "permission denied",
          "status display text remains human readable");
  require(archspec::available(archspec::Status::ok), "ok status is available");
  require(!archspec::available(archspec::Status::not_found), "missing status is unavailable");

  archspec::StringField missing =
      archspec::StringField::unavailable(archspec::Status::not_found);
  require(missing.value_or("fallback") == "fallback", "Field::value_or fallback");
  require(!missing && missing.status() == archspec::Status::not_found,
          "invalid field preserves its status");

  std::uint64_t number = 0;
  require(archspec::detail::parse_u64("0xff", number) && number == 255,
          "unsigned parser accepts procfs numeric syntax");
  require(!archspec::detail::parse_u64("-1", number),
          "unsigned parser rejects negative values");
  require(!archspec::detail::parse_u64("1 trailing", number),
          "unsigned parser rejects trailing data");
  require(!archspec::detail::parse_size_bytes("-1K", number),
          "size parser rejects negative values");
  require(archspec::detail::cpu_list_count("0-2,8,10-11").value() == 6,
          "CPU list parser counts ranges");
  require(!archspec::detail::cpu_list_count("0-18446744073709551615"),
          "CPU list parser detects count overflow");
  require(!archspec::detail::cpu_list_count("4-3"), "CPU list parser rejects reversed ranges");

  const std::filesystem::path fixture =
      std::filesystem::temp_directory_path() / "archspec-core-tests";
  std::filesystem::remove_all(fixture);
  make_fixture(fixture);

  archspec::CollectOptions options;
  options.procfs_root = (fixture / "proc").string();
  options.sysfs_root = (fixture / "sys").string();
  options.etc_root = (fixture / "etc").string();
  options.dev_root = (fixture / "dev").string();
  options.include_sensitive = false;
  const archspec::Collector collector{options};

  const archspec::OsInfo os = collector.collect_os();
  require(os.distro_id.valid() && os.distro_id.value() == "fixture",
          "OS collector reads injected os-release");
  require(os.hostname.status() == archspec::Status::redacted &&
              os.cmdline.status() == archspec::Status::redacted,
          "OS collector redacts sensitive fields");

  const archspec::CpuInfo cpu = collector.collect_cpu();
  require(cpu.online_cpu_count.valid() && cpu.online_cpu_count.value() == 2,
          "CPU collector parses CPU list");
  require(cpu.topology.size() == 2 && cpu.sockets.value_or(0) == 1 &&
              cpu.threads_per_core.value_or(0) == 2,
          "CPU collector builds topology");
  require(cpu.physical_address_bits.value_or(0) == 46 &&
              cpu.virtual_address_bits.value_or(0) == 57,
          "CPU collector parses address sizes");

  const archspec::IsaFeatures isa = collector.collect_isa();
  require(isa.x86_aes.valid() && isa.x86_aes.value() && isa.x86_avx2.value(),
          "ISA collector detects x86 flags");
  require(!isa.arm_aes.valid() && isa.arm_aes.status() == archspec::Status::unsupported,
          "ISA collector isolates architecture families");
  write_file(fixture, "proc/riscv/cpuinfo", "isa : rv64g\n");
  archspec::CollectOptions riscv_options = options;
  riscv_options.procfs_root = (fixture / "proc/riscv").string();
  const archspec::IsaFeatures riscv = archspec::Collector{riscv_options}.collect_isa();
  require(riscv.riscv_i.value_or(false) && riscv.riscv_m.value_or(false) &&
              riscv.riscv_a.value_or(false) && riscv.riscv_f.value_or(false) &&
              riscv.riscv_d.value_or(false) && !riscv.riscv_c.value_or(true),
          "ISA collector expands the RISC-V G extension correctly");

  const archspec::CacheList cache = collector.collect_cache();
  require(cache.entries.size() == 1 && cache.entries[0].size.value_or(0) == 32768,
          "cache collector parses and deduplicates shared caches");
  std::filesystem::remove(
      fixture / "sys/devices/system/cpu/cpu0/cache/index0/shared_cpu_list"
  );
  std::filesystem::remove(
      fixture / "sys/devices/system/cpu/cpu1/cache/index0/shared_cpu_list"
  );
  require(collector.collect_cache().entries.size() == 2,
          "cache collector retains independent caches with unavailable sharing data");
  const archspec::MemoryInfo memory = collector.collect_memory();
  require(memory.total_kb.value_or(0) == 1024 && memory.numa_nodes.size() == 1,
          "memory collector parses memory and NUMA fixtures");
  require(memory.transparent_hugepages_enabled.value_or(false),
          "memory collector detects selected transparent hugepage mode");

  const archspec::PciDeviceList pci = collector.collect_pci();
  require(pci.entries.size() == 1 && pci.entries[0].pci_address.value() == "0000:00:02.0",
          "PCI collector reads injected PCI tree");
  const archspec::GpuList gpu = collector.collect_gpu();
  require(gpu.entries.size() == 1 && gpu.entries[0].vendor == archspec::GpuVendor::intel,
          "GPU collector finds display-class PCI devices");
  require(gpu.entries[0].vram_total_bytes.status() == archspec::Status::parse_error,
          "GPU collector preserves a malformed VRAM status");
  require(gpu.entries[0].power_mw.value_or(0) == 1234,
          "GPU collector converts microwatts to milliwatts");

  const archspec::PerfCounterInfo perf = collector.collect_perf();
  require(perf.perf_event_paranoid.value_or(0) == 2 && perf.rdtsc_available.value_or(false),
          "performance collector parses fixture data without probing");
  require(perf.available.cpu_cycles.status() == archspec::Status::unsupported,
          "performance probes remain opt-in");
  const archspec::BlockDeviceList block = collector.collect_block();
  require(block.entries.size() == 1 && block.entries[0].size_bytes.value_or(0) == 1024,
          "block collector converts sectors to bytes");
  const archspec::NetInterfaceList net = collector.collect_net();
  require(net.entries.size() == 1 && net.entries[0].speed_mbps.status() == archspec::Status::not_found,
          "network collector handles unavailable link speed");
  require(net.entries[0].mac_address.status() == archspec::Status::redacted &&
              net.entries[0].ipv4_addresses.status() == archspec::Status::redacted,
          "network collector redacts fixture data without reading host addresses");
  const archspec::ThermalInfo thermal = collector.collect_thermal();
  require(thermal.entries.size() == 1 && thermal.entries[0].temp.value_or("") == "42000",
          "thermal collector reads zones");
  const archspec::PowerInfo power = collector.collect_power();
  require(power.on_battery.value_or(false) && power.battery_capacity_percent.value_or(0) == 73,
          "power collector reads battery state");
  require(power.cpu_current_freq_mhz.status() == archspec::Status::parse_error,
          "power collector preserves malformed frequency status");
  const archspec::VirtualizationInfo virtualization = collector.collect_virtualization();
  require(virtualization.running_under_hypervisor.value_or(false) &&
              virtualization.intel_vmx_available.value_or(false) &&
              virtualization.iommu_present.value_or(false),
          "virtualization collector combines CPU and sysfs evidence");
  const archspec::PlatformInfo platform = collector.collect_platform();
  require(platform.product_name.value_or("") == "Fixture Machine" &&
              platform.secure_boot.status() == archspec::Status::not_found,
          "platform collector reads DMI and preserves absent EFI status");

  const archspec::Report report = archspec::collect_report(options);
  const std::string json = archspec::to_json(report);
  require(json.find("\"schema_version\":\"1.0.0\"") != std::string::npos,
          "report declares its schema version");
  require(json.find("\"collected_categories\":[\"os\",\"cpu\"") != std::string::npos,
          "report lists selected categories in a stable order");
  require(json.find("\"valid\":false,\"status\":\"ok\"") == std::string::npos,
          "serialized invalid fields never claim ok status");

  archspec::OsInfo escaped;
  escaped.kernel_name = archspec::StringField::value("quote=\" newline=\n control=\x01");
  const std::string escaped_json = archspec::to_json(escaped);
  require(escaped_json.find("quote=\\\"") != std::string::npos &&
              escaped_json.find("newline=\\n") != std::string::npos &&
              escaped_json.find("\\u0001") != std::string::npos,
          "JSON strings escape quotes, newlines, and control characters");

  std::filesystem::remove_all(fixture);
  std::cout << "All core checks passed\n";
  return 0;
}
