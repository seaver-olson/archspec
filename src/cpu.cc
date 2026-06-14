#include "aview.hh"

#include <sys/utsname.h>
#include <unistd.h>
#include <fstream>

namespace archspec {
namespace {

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

CpuInfo Collector::collect_cpu() const {
  CpuInfo info;
  info.arch = ArchType::unknown;

  struct utsname uts {};
  if (uname(&uts) == 0) {
    std::string machine = uts.machine;
    info.arch_name = StringField::value(machine);
    info.arch = arch_from_machine(machine);
  } else {
    info.arch_name = StringField::unavailable(Status::internal_error);
  }


  // TODO: parse /proc/cpuinfo for vendor, brand, family, model, stepping, microcode.
  info.vendor = StringField::unavailable(Status::unsupported);
  info.brand = StringField::unavailable(Status::unsupported);
  info.model_name = StringField::unavailable(Status::unsupported);

  info.sockets = U64Field::unavailable(Status::unsupported);
  info.cores_per_socket = U64Field::unavailable(Status::unsupported);
  info.threads_per_core = U64Field::unavailable(Status::unsupported);

  info.family = U64Field::unavailable(Status::unsupported);
  info.model = U64Field::unavailable(Status::unsupported);
  info.stepping = U64Field::unavailable(Status::unsupported);
  info.microcode = StringField::unavailable(Status::unsupported);

  info.physical_address_bits = U64Field::unavailable(Status::unsupported);
  info.virtual_address_bits = U64Field::unavailable(Status::unsupported);

  info.is_virtualized = BoolField::unavailable(Status::unsupported);
  info.hypervisor_vendor = StringField::unavailable(Status::unsupported);

  return info;
}


CpuInfo collect_cpu() { return Collector{}.collect_cpu();}

}