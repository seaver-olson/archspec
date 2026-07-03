#include "aview.hh"
#include "internal.hh"

#include <cerrno>
#include <cstdint>
#include <set>

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace archspec {
namespace {

struct CpuFlagResult {
  Status status = Status::not_found;
  std::set<std::string> flags;

  bool valid() const { return status == Status::ok; }
};

CpuFlagResult read_cpu_flags(const CollectOptions& options) {
  detail::ReadResult cpuinfo = detail::read_file(detail::proc_path(options, "/cpuinfo"));
  if (cpuinfo.status != Status::ok) {
    return {cpuinfo.status, {}};
  }

  std::vector<detail::KeyValueMap> blocks = detail::parse_colon_blocks(cpuinfo.value);
  if (blocks.empty()) {
    return {Status::parse_error, {}};
  }

  for (const char* key : {"flags", "Features", "isa"}) {
    auto value = detail::map_value(blocks.front(), key);
    if (value) {
      return {Status::ok, detail::word_set(*value)};
    }
  }

  return {Status::not_found, {}};
}

I64Field read_perf_event_paranoid(const CollectOptions& options) {
  detail::ReadResult result = detail::read_first_line(
      detail::proc_path(options, "/sys/kernel/perf_event_paranoid")
  );
  if (result.status != Status::ok) {
    return I64Field::unavailable(result.status);
  }

  std::int64_t value = 0;
  if (!detail::parse_i64(result.value, value)) {
    return I64Field::unavailable(Status::parse_error);
  }

  return I64Field::value(value);
}

BoolField flag_field(const CpuFlagResult& result, const std::string& name) {
  if (!result.valid()) {
    return BoolField::unavailable(result.status);
  }

  return detail::bool_from_flag(result.flags, name);
}

BoolField any_flag_field(const CpuFlagResult& result, const std::vector<std::string>& names) {
  if (!result.valid()) {
    return BoolField::unavailable(result.status);
  }

  for (const std::string& name : names) {
    if (result.flags.find(name) != result.flags.end()) {
      return BoolField::value(true);
    }
  }

  return BoolField::value(false);
}

BoolField probe_hardware_counter(std::uint64_t config, bool allow_probe) {
  if (!allow_probe) {
    return BoolField::unavailable(Status::unsupported);
  }

#if defined(__linux__) && defined(__NR_perf_event_open)
  perf_event_attr attr {};
  attr.type = PERF_TYPE_HARDWARE;
  attr.size = sizeof(attr);
  attr.config = config;
  attr.disabled = 1;
  attr.exclude_kernel = 1;
  attr.exclude_hv = 1;

  errno = 0;
  int fd = static_cast<int>(syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0));
  if (fd >= 0) {
    close(fd);
    return BoolField::value(true);
  }

  switch (errno) {
    case EACCES:
    case EPERM:
      return BoolField::unavailable(Status::perm_denied);
    case ENODEV:
    case ENOENT:
    case EOPNOTSUPP:
    case EINVAL:
      return BoolField::value(false);
    default:
      return BoolField::unavailable(Status::internal_error);
  }
#else
  (void)config;
  return BoolField::unavailable(Status::unsupported);
#endif
}

void fill_hardware_counter_availability(
    PerfCounterAvailability& available,
    bool allow_probe
) {
#if defined(__linux__) && defined(__NR_perf_event_open)
  available.cpu_cycles = probe_hardware_counter(PERF_COUNT_HW_CPU_CYCLES, allow_probe);
  available.instructions = probe_hardware_counter(PERF_COUNT_HW_INSTRUCTIONS, allow_probe);
  available.cache_references = probe_hardware_counter(
      PERF_COUNT_HW_CACHE_REFERENCES,
      allow_probe
  );
  available.cache_misses = probe_hardware_counter(PERF_COUNT_HW_CACHE_MISSES, allow_probe);
  available.branch_instructions = probe_hardware_counter(
      PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
      allow_probe
  );
  available.branch_misses = probe_hardware_counter(PERF_COUNT_HW_BRANCH_MISSES, allow_probe);
  available.bus_cycles = probe_hardware_counter(PERF_COUNT_HW_BUS_CYCLES, allow_probe);
  available.ref_cpu_cycles = probe_hardware_counter(PERF_COUNT_HW_REF_CPU_CYCLES, allow_probe);
#else
  (void)allow_probe;
  available.cpu_cycles = BoolField::unavailable(Status::unsupported);
  available.instructions = BoolField::unavailable(Status::unsupported);
  available.cache_references = BoolField::unavailable(Status::unsupported);
  available.cache_misses = BoolField::unavailable(Status::unsupported);
  available.branch_instructions = BoolField::unavailable(Status::unsupported);
  available.branch_misses = BoolField::unavailable(Status::unsupported);
  available.bus_cycles = BoolField::unavailable(Status::unsupported);
  available.ref_cpu_cycles = BoolField::unavailable(Status::unsupported);
#endif
}

} // namespace

PerfCounterInfo Collector::collect_perf() const {
  PerfCounterInfo info;

  info.perf_event_paranoid = read_perf_event_paranoid(options_);
  fill_hardware_counter_availability(info.available, options_.allow_perf_open);

  CpuFlagResult flags = read_cpu_flags(options_);
  info.rdtsc_available = flag_field(flags, "tsc");
  info.rdtscp_available = flag_field(flags, "rdtscp");
  info.invariant_tsc = any_flag_field(flags, {"constant_tsc", "nonstop_tsc"});

  return info;
}

PerfCounterInfo collect_perf() { return Collector{}.collect_perf(); }

} // namespace archspec
