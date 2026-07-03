#include "aview.hh"
#include "internal.hh"

namespace archspec {

PciDeviceList Collector::collect_pci() const {
  PciDeviceList list;
  std::string root = detail::sys_path(options_, "/bus/pci/devices");

  for (const std::string& address : detail::list_dir(root)) {
    std::string path = detail::join_path(root, address);
    if (!detail::path_is_dir(path)) {
      continue;
    }

    PciDeviceInfo device;
    device.pci_address = StringField::value(address);
    device.vendor_id = detail::read_string_field(detail::join_path(path, "vendor"));
    device.device_id = detail::read_string_field(detail::join_path(path, "device"));
    device.subsystem_vendor_id = detail::read_string_field(
        detail::join_path(path, "subsystem_vendor")
    );
    device.subsystem_device_id = detail::read_string_field(
        detail::join_path(path, "subsystem_device")
    );
    device.class_id = detail::read_string_field(detail::join_path(path, "class"));
    device.driver = detail::read_symlink_basename_field(detail::join_path(path, "driver"));
    device.numa_node = detail::read_i64_field(detail::join_path(path, "numa_node"));
    device.iommu_group = detail::read_symlink_basename_field(
        detail::join_path(path, "iommu_group")
    );

    std::string resource = detail::join_path(path, "resource");
    if (detail::path_exists(resource)) {
      device.resource_path = StringField::value(resource);
    } else {
      device.resource_path = StringField::unavailable(Status::not_found);
    }

    list.entries.push_back(device);
  }

  return list;
}

PciDeviceList collect_pci() { return Collector{}.collect_pci(); }

} // namespace archspec
