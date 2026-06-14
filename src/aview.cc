#include "aview.hh"

namespace archspec {

std::string to_string(Status status) {
  switch (status){
    case Status::ok: return "ok";
    case Status::unsupported: return "unsupported";
    case Status::perm_denied: return "permission denied";
    case Status::not_found: return "not found";
    case Status::parse_error: return "parse error";
    case Status::invalid_arg: return "invalid argument";
    case Status::internal_error: return "internal error";
    default: return "unknown";
  }
}

std::string to_string(ArchType arch) {
  switch (arch){
    case ArchType::unknown: return "unknown";
    case ArchType::x86: return "x86";
    case ArchType::x86_64: return "x86_64";
    case ArchType::arm: return "arm";
    case ArchType::aarch64: return "aarch64";
    case ArchType::riscv32: return "riscv32";
    case ArchType::riscv64: return "riscv64";
    case ArchType::ppc64: return "ppc64";
    case ArchType::s390x: return "s390x";
    default: return "unknown";
  }
}

std::string to_string(CacheType type) {
  switch (type){
    case CacheType::unknown: return "unknown";
    case CacheType::data: return "data";
    case CacheType::instruction: return "instruction";
    case CacheType::unified: return "unified";
    default: return "unknown";
  }
}

std::string to_string(GpuVendor vendor) {
  switch (vendor){
    case GpuVendor::unknown: return "unknown";
    case GpuVendor::intel: return "intel";
    case GpuVendor::amd: return "amd";
    case GpuVendor::nvidia: return "nvidia";
    default: return "unknown";
  }
}

bool available(Status status) {
  return status == Status::ok;
}

}