#include "aview.hh"
#include "internal.hh"

namespace archspec {
namespace {

BoolField collect_secure_boot(const CollectOptions& options) {
  std::string efivars = detail::sys_path(options, "/firmware/efi/efivars");
  for (const std::string& name : detail::list_dir(efivars)) {
    if (!detail::starts_with(name, "SecureBoot-")) {
      continue;
    }

    detail::ReadResult value = detail::read_file(
        detail::join_path(efivars, name),
        std::ios::in | std::ios::binary
    );
    if (value.status != Status::ok) {
      return BoolField::unavailable(value.status);
    }

    if (value.value.size() >= 5) {
      return BoolField::value(static_cast<unsigned char>(value.value[4]) != 0);
    }

    return BoolField::unavailable(Status::parse_error);
  }

  return BoolField::unavailable(Status::not_found);
}

} // namespace

PlatformInfo Collector::collect_platform() const {
  PlatformInfo info;
  std::string dmi = detail::sys_path(options_, "/class/dmi/id");

  info.system_vendor = detail::read_string_field(detail::join_path(dmi, "sys_vendor"));
  info.product_name = detail::read_string_field(detail::join_path(dmi, "product_name"));
  info.product_version = detail::read_string_field(detail::join_path(dmi, "product_version"));

  info.board_vendor = detail::read_string_field(detail::join_path(dmi, "board_vendor"));
  info.board_name = detail::read_string_field(detail::join_path(dmi, "board_name"));

  info.bios_vendor = detail::read_string_field(detail::join_path(dmi, "bios_vendor"));
  info.bios_version = detail::read_string_field(detail::join_path(dmi, "bios_version"));
  info.bios_date = detail::read_string_field(detail::join_path(dmi, "bios_date"));

  bool uefi = detail::path_is_dir(detail::sys_path(options_, "/firmware/efi"));
  info.uefi = BoolField::value(uefi);
  info.secure_boot = uefi ? collect_secure_boot(options_)
                          : BoolField::unavailable(Status::not_found);

  return info;
}

PlatformInfo collect_platform() { return Collector{}.collect_platform(); }

} // namespace archspec
