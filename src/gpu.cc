#include "aview.hh"
#include "internal.hh"

#include <map>
#include <set>

namespace archspec {
namespace {

GpuVendor vendor_from_id(const StringField& vendor_id) {
  if (!vendor_id.valid()) {
    return GpuVendor::unknown;
  }

  std::string id = vendor_id.value();
  if (id == "0x8086") {
    return GpuVendor::intel;
  }
  if (id == "0x1002" || id == "0x1022") {
    return GpuVendor::amd;
  }
  if (id == "0x10de") {
    return GpuVendor::nvidia;
  }

  return GpuVendor::unknown;
}

bool is_display_class(const StringField& class_id) {
  return class_id.valid() && detail::starts_with(class_id.value(), "0x03");
}

std::map<std::string, std::string> render_nodes_by_pci(const CollectOptions& options) {
  std::map<std::string, std::string> nodes;
  std::string drm_root = detail::sys_path(options, "/class/drm");

  for (const std::string& name : detail::list_dir(drm_root)) {
    if (!detail::starts_with(name, "renderD")) {
      continue;
    }

    std::optional<std::string> target = detail::read_symlink_target(
        detail::join_path(drm_root, name + "/device")
    );
    if (target) {
      nodes[detail::basename(*target)] = detail::dev_path(options, "/dri/" + name);
    }
  }

  return nodes;
}

U64Field first_u64_file(const std::vector<std::string>& paths) {
  Status last_status = Status::not_found;
  for (const std::string& path : paths) {
    U64Field value = detail::read_u64_field(path);
    if (value.valid()) {
      return value;
    }
    if (value.status() != Status::not_found) {
      last_status = value.status();
    }
  }

  return U64Field::unavailable(last_status);
}

void fill_hwmon_fields(const std::string& pci_path, GpuInfo& gpu) {
  std::string hwmon_root = detail::join_path(pci_path, "hwmon");
  for (const std::string& hwmon : detail::list_dir(hwmon_root)) {
    std::string path = detail::join_path(hwmon_root, hwmon);
    if (!gpu.temp_millidegree_c.valid()) {
      gpu.temp_millidegree_c = detail::read_u64_field(detail::join_path(path, "temp1_input"));
    }
    if (!gpu.power_mw.valid()) {
      U64Field power_uw = detail::read_u64_field(detail::join_path(path, "power1_average"));
      if (power_uw.valid()) {
        gpu.power_mw = U64Field::value(power_uw.value() / 1000);
      }
    }
  }
}

GpuInfo gpu_from_pci(
    const CollectOptions& options,
    const std::string& pci_address,
    const std::string& pci_path,
    const std::map<std::string, std::string>& render_nodes
) {
  GpuInfo gpu;
  gpu.pci_address = StringField::value(pci_address);
  gpu.vendor_id = detail::read_string_field(detail::join_path(pci_path, "vendor"));
  gpu.device_id = detail::read_string_field(detail::join_path(pci_path, "device"));
  gpu.driver = detail::read_symlink_basename_field(detail::join_path(pci_path, "driver"));
  gpu.vendor = vendor_from_id(gpu.vendor_id);

  if (gpu.vendor_id.valid() && gpu.device_id.valid()) {
    gpu.name = StringField::value(gpu.vendor_id.value() + ":" + gpu.device_id.value());
  } else {
    gpu.name = StringField::value(pci_address);
  }

  auto render = render_nodes.find(pci_address);
  if (render != render_nodes.end()) {
    gpu.render_node = StringField::value(render->second);
  }

  gpu.vram_total_bytes = first_u64_file({
      detail::join_path(pci_path, "mem_info_vram_total"),
      detail::join_path(pci_path, "mem_info_vis_vram_total"),
  });
  gpu.vram_used_bytes = first_u64_file({
      detail::join_path(pci_path, "mem_info_vram_used"),
      detail::join_path(pci_path, "mem_info_vis_vram_used"),
  });

  fill_hwmon_fields(pci_path, gpu);

  // A NVIDIA PCI device does not prove that a usable CUDA runtime is present.
  // Vendor-library probing remains opt-in and is not implemented yet.
  gpu.cuda_available = gpu.vendor == GpuVendor::nvidia
      ? BoolField::unavailable(Status::unsupported)
      : BoolField::value(false);
  gpu.cuda_compute_capability = StringField::unavailable(Status::unsupported);

  (void)options;
  return gpu;
}

} // namespace

GpuList Collector::collect_gpu() const {
  GpuList list;
  std::string drm_root = detail::sys_path(options_, "/class/drm");
  std::string pci_root = detail::sys_path(options_, "/bus/pci/devices");
  std::map<std::string, std::string> render_nodes = render_nodes_by_pci(options_);
  std::set<std::string> seen;

  for (const std::string& card : detail::list_dir(drm_root)) {
    if (!detail::starts_with(card, "card") || card.find('-') != std::string::npos) {
      continue;
    }

    std::optional<std::string> target = detail::read_symlink_target(
        detail::join_path(drm_root, card + "/device")
    );
    if (!target) {
      continue;
    }

    std::string pci_address = detail::basename(*target);
    if (!seen.insert(pci_address).second) {
      continue;
    }
    std::string pci_path = detail::join_path(pci_root, pci_address);
    GpuInfo gpu = gpu_from_pci(options_, pci_address, pci_path, render_nodes);
    gpu.drm_card = StringField::value(detail::dev_path(options_, "/dri/" + card));

    list.entries.push_back(gpu);
  }

  for (const std::string& address : detail::list_dir(pci_root)) {
    if (seen.find(address) != seen.end()) {
      continue;
    }

    std::string pci_path = detail::join_path(pci_root, address);
    StringField class_id = detail::read_string_field(detail::join_path(pci_path, "class"));
    if (!is_display_class(class_id)) {
      continue;
    }

    list.entries.push_back(gpu_from_pci(options_, address, pci_path, render_nodes));
  }

  return list;
}

GpuList collect_gpu() { return Collector{}.collect_gpu(); }

} // namespace archspec
