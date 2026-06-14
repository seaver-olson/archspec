#include "aview.hh"

#include <sys/utsname.h>
#include <unistd.h>
#include <fstream>


namespace archspec {
//private helpers
namespace {
std::string read_first_line(const std::string& path){
  std::ifstream file(path);
  std::string line;

  if (!file.is_open()){
    return {};
  }

  std::getline(file, line);
  return line;
}

ArchType arch_from_machine(const std::string& machine){
  if (machine == "x86_64" || machine == "amd64"){
    return ArchType::x86_64;
  } else if (machine == "i386" || machine == "i686"){
    return ArchType::x86;
  } else if (machine == "armv7l" || machine == "armv8l"){
    return ArchType::arm;
  } else if (machine == "aarch64"){
    return ArchType::aarch64;
  } else if (machine == "riscv32"){
    return ArchType::riscv32;
  } else if (machine == "riscv64"){
    return ArchType::riscv64;
  } else if (machine == "ppc64le"){
    return ArchType::ppc64;
  } else if (machine == "s390x"){
    return ArchType::s390x;
  }

  return ArchType::unknown;
}
}
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

  // Check endianness
  std::uint16_t test = 1;
  bool little = *reinterpret_cast<unsigned char*>(&test) == 1;
  info.endianness = StringField::value(little ? "little" : "big");

  info.cmdline = StringField::value(read_first_line("/proc/cmdline"));

  // Minimal /etc/os-release parsing coming soon...
  info.distro_name = StringField::unavailable(Status::unsupported);
  info.distro_version = StringField::unavailable(Status::unsupported);
  info.distro_id = StringField::unavailable(Status::unsupported);

  return info;
}

OsInfo collect_os() { return Collector{}.collect_os();}

}