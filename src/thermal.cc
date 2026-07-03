#include "aview.hh"
#include "internal.hh"

namespace archspec {

ThermalInfo Collector::collect_thermal() const {
  ThermalInfo info;
  std::string root = detail::sys_path(options_, "/class/thermal");

  for (const std::string& name : detail::list_dir(root)) {
    if (!detail::starts_with(name, "thermal_zone")) {
      continue;
    }

    std::string path = detail::join_path(root, name);
    ThermalZoneInfo zone;
    zone.name = StringField::value(name);
    zone.type = detail::read_string_field(detail::join_path(path, "type"));
    zone.temp = detail::read_string_field(detail::join_path(path, "temp"));
    info.entries.push_back(zone);
  }

  return info;
}

ThermalInfo collect_thermal() { return Collector{}.collect_thermal(); }

} // namespace archspec
