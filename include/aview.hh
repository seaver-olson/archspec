#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace archspec {
enum class Status {
  ok,
  unsupported,
  perm_denied,
  not_found,
  parse_error,
  invalid_arg,
  internal_error
};

std::string to_string(Status status);

template <typename T>
class Field {
public:
  Field() = default;

  static Field<T> value(T value) {
    Field<T> field;
    field.valid_ = true;
    field.status_ = Status::ok;
    field.value_ = std::move(value);
    return field;
  }

  static Field<T> unavailable(Status status) {
    Field<T> field;
    field.valid_ = false;
    field.status_ = status;
    return field;
  }

  bool valid() const { return valid_; }

  explicit operator bool() const { return valid_; }

  const T& value() const { return value_; }

  T& get() { return value_; }
  
  // Return the value if valid, otherwise return the provided default value.
  const T& value_or(const T& default_value) const {
    return valid_ ? value_ : default_value;
  }

  Status status() const { return status_; }
private:
  bool valid_ = false;
  Status status_ = Status::not_found;
  T value_{};
};

// Architecture

enum class ArchType{
  unknown,
  x86,
  x86_64,
  arm,
  aarch64,
  riscv32,
  riscv64,
  ppc64,
  s390x
};

std::string to_string(ArchType arch);

// OS info


// CPU info

// ISA info

// Cache info
// Memory / NUMA info
// PCI info
// GPU info
// Performance Counter info
// Block devices
// Network
// Thermal / Power
// Virtualization / platform
// Full system info
// Collection categories
//
enum class CollectCategory : std::uint64_t {
    none           = 0,

    os             = 1ull << 0,
    cpu            = 1ull << 1,
    isa            = 1ull << 2,
    cache          = 1ull << 3,
    memory         = 1ull << 4,
    pci            = 1ull << 5,
    gpu            = 1ull << 6,
    perf           = 1ull << 7,
    block          = 1ull << 8,
    net            = 1ull << 9,
    thermal        = 1ull << 10,
    power          = 1ull << 11,
    virtualization = 1ull << 12,
    platform       = 1ull << 13,

    all            = ~0ull
};

}
