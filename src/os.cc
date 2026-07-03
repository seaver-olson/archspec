#include "aview.hh"
#include "internal.hh"

#include <sys/utsname.h>
#include <unistd.h>
#include <map>

namespace archspec {
namespace {

detail::KeyValueMap parse_os_release(const std::string& text) {
  detail::KeyValueMap values;
  std::istringstream in(text);
  std::string line;

  while (std::getline(in, line)) {
    line = detail::trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }

    values[detail::trim(line.substr(0, equals))] =
        detail::unquote_value(line.substr(equals + 1));
  }

  return values;
}

StringField os_release_field(
    const detail::KeyValueMap& values,
    const std::vector<std::string>& keys
) {
  return detail::string_from_map(values, keys);
}

} // namespace

OsInfo Collector::collect_os() const {
  OsInfo info;

  struct utsname uts {};
  if (uname(&uts) == 0) {
    info.kernel_name = StringField::value(uts.sysname);
    info.kernel_release = StringField::value(uts.release);
    info.kernel_version = StringField::value(uts.version);
    info.machine = StringField::value(uts.machine);
    info.hostname = StringField::value(uts.nodename);
  } else {
    info.kernel_name = StringField::unavailable(Status::internal_error);
    info.kernel_release = StringField::unavailable(Status::internal_error);
    info.kernel_version = StringField::unavailable(Status::internal_error);
    info.machine = StringField::unavailable(Status::internal_error);
    info.hostname = StringField::unavailable(Status::internal_error);
  }

  long page = sysconf(_SC_PAGESIZE);
  if (page > 0) {
    info.page_size = U64Field::value(static_cast<std::uint64_t>(page));
  } else {
    info.page_size = U64Field::unavailable(Status::not_found);
  }

  info.word_size = U64Field::value(static_cast<std::uint64_t>(sizeof(void*) * 8));

  std::uint16_t test = 1;
  bool little = *reinterpret_cast<unsigned char*>(&test) == 1;
  info.endianness = StringField::value(little ? "little" : "big");

  info.cmdline = detail::read_string_field(
      detail::proc_path(options_, "/cmdline"),
      false
  );

  detail::ReadResult os_release = detail::read_file(detail::etc_path(options_, "/os-release"));
  if (os_release.status != Status::ok) {
    info.distro_name = StringField::unavailable(os_release.status);
    info.distro_version = StringField::unavailable(os_release.status);
    info.distro_id = StringField::unavailable(os_release.status);
  } else {
    detail::KeyValueMap values = parse_os_release(os_release.value);
    info.distro_name = os_release_field(values, {"PRETTY_NAME", "NAME"});
    info.distro_version = os_release_field(values, {"VERSION_ID", "VERSION"});
    info.distro_id = os_release_field(values, {"ID"});
  }

  return info;
}

OsInfo collect_os() { return Collector{}.collect_os();}

}
