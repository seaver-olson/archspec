#include "aview.hh"
#include "internal.hh"

namespace archspec {
namespace {

U64Field read_khz_as_mhz(const std::vector<std::string>& paths) {
  for (const std::string& path : paths) {
    U64Field khz = detail::read_u64_field(path);
    if (khz.valid()) {
      return U64Field::value(khz.value() / 1000);
    }
  }

  return U64Field::unavailable(Status::not_found);
}

bool is_mains_type(const std::string& type) {
  return type == "Mains" || type == "AC" || type == "USB" || type == "USB_C";
}

} // namespace

PowerInfo Collector::collect_power() const {
  PowerInfo info;
  std::string root = detail::sys_path(options_, "/class/power_supply");
  bool saw_battery = false;
  bool saw_mains = false;
  bool any_mains_online = false;

  for (const std::string& name : detail::list_dir(root)) {
    std::string path = detail::join_path(root, name);
    StringField type = detail::read_string_field(detail::join_path(path, "type"));
    if (!type.valid()) {
      continue;
    }

    if (type.value() == "Battery") {
      saw_battery = true;
      if (!info.battery_status.valid()) {
        info.battery_status = detail::read_string_field(detail::join_path(path, "status"));
      }
      if (!info.battery_capacity_percent.valid()) {
        info.battery_capacity_percent = detail::read_u64_field(
            detail::join_path(path, "capacity")
        );
      }
    } else if (is_mains_type(type.value())) {
      saw_mains = true;
      U64Field online = detail::read_u64_field(detail::join_path(path, "online"));
      if (online.valid() && online.value() != 0) {
        any_mains_online = true;
      }
    }
  }

  if (saw_battery) {
    if (saw_mains) {
      info.on_battery = BoolField::value(!any_mains_online);
    } else if (info.battery_status.valid()) {
      std::string status = info.battery_status.value();
      info.on_battery = BoolField::value(status == "Discharging");
    } else {
      info.on_battery = BoolField::unavailable(Status::not_found);
    }
  } else if (saw_mains) {
    info.on_battery = BoolField::value(false);
  } else {
    info.on_battery = BoolField::unavailable(Status::not_found);
  }

  std::string cpufreq = detail::sys_path(options_, "/devices/system/cpu/cpu0/cpufreq");
  info.cpu_governor = detail::read_string_field(
      detail::join_path(cpufreq, "scaling_governor")
  );
  info.cpu_current_freq_mhz = read_khz_as_mhz({
      detail::join_path(cpufreq, "scaling_cur_freq"),
      detail::join_path(cpufreq, "cpuinfo_cur_freq"),
  });
  info.cpu_min_freq_mhz = read_khz_as_mhz({
      detail::join_path(cpufreq, "scaling_min_freq"),
      detail::join_path(cpufreq, "cpuinfo_min_freq"),
  });
  info.cpu_max_freq_mhz = read_khz_as_mhz({
      detail::join_path(cpufreq, "scaling_max_freq"),
      detail::join_path(cpufreq, "cpuinfo_max_freq"),
  });

  return info;
}

PowerInfo collect_power() { return Collector{}.collect_power(); }

} // namespace archspec
