#include "aview.hh"
#include "internal.hh"

#include <algorithm>
#include <set>

namespace archspec {
namespace {

std::string find_raw_flags(const detail::KeyValueMap& block) {
  for (const char* key : {"flags", "Features", "isa"}) {
    auto value = detail::map_value(block, key);
    if (value) {
      return *value;
    }
  }

  return {};
}

BoolField any_flag(const std::set<std::string>& flags, const std::vector<std::string>& names) {
  for (const std::string& name : names) {
    if (flags.find(name) != flags.end()) {
      return BoolField::value(true);
    }
  }

  return BoolField::value(false);
}

bool riscv_has_extension(const std::string& isa, char extension) {
  std::string lower = isa;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  std::size_t start = lower.find("rv32");
  if (start == std::string::npos) {
    start = lower.find("rv64");
  }
  if (start == std::string::npos) {
    return false;
  }

  start += 4;
  std::size_t end = lower.find('_', start);
  std::string base = lower.substr(
      start,
      end == std::string::npos ? std::string::npos : end - start
  );

  return base.find(extension) != std::string::npos ||
         lower.find(std::string("_") + extension) != std::string::npos;
}

} // namespace

IsaFeatures Collector::collect_isa() const {
  IsaFeatures features;

  detail::ReadResult cpuinfo = detail::read_file(detail::proc_path(options_, "/cpuinfo"));
  if (cpuinfo.status != Status::ok) {
    features.raw_flags = StringField::unavailable(cpuinfo.status);
    return features;
  }

  std::vector<detail::KeyValueMap> blocks = detail::parse_colon_blocks(cpuinfo.value);
  if (blocks.empty()) {
    features.raw_flags = StringField::unavailable(Status::parse_error);
    return features;
  }

  std::string raw = find_raw_flags(blocks.front());
  if (raw.empty()) {
    features.raw_flags = StringField::unavailable(Status::not_found);
    return features;
  }

  features.raw_flags = StringField::value(raw);
  std::set<std::string> flags = detail::word_set(raw);

  features.x86_fpu = detail::bool_from_flag(flags, "fpu");
  features.x86_mmx = detail::bool_from_flag(flags, "mmx");
  features.x86_sse = detail::bool_from_flag(flags, "sse");
  features.x86_sse2 = detail::bool_from_flag(flags, "sse2");
  features.x86_sse3 = any_flag(flags, {"sse3", "pni"});
  features.x86_ssse3 = detail::bool_from_flag(flags, "ssse3");
  features.x86_sse41 = any_flag(flags, {"sse4_1", "sse4.1"});
  features.x86_sse42 = any_flag(flags, {"sse4_2", "sse4.2"});

  features.x86_avx = detail::bool_from_flag(flags, "avx");
  features.x86_avx2 = detail::bool_from_flag(flags, "avx2");
  features.x86_avx512f = detail::bool_from_flag(flags, "avx512f");
  features.x86_avx512dq = detail::bool_from_flag(flags, "avx512dq");
  features.x86_avx512cd = detail::bool_from_flag(flags, "avx512cd");
  features.x86_avx512bw = detail::bool_from_flag(flags, "avx512bw");
  features.x86_avx512vl = detail::bool_from_flag(flags, "avx512vl");

  features.x86_aes = detail::bool_from_flag(flags, "aes");
  features.x86_sha = any_flag(flags, {"sha", "sha_ni"});
  features.x86_pclmulqdq = detail::bool_from_flag(flags, "pclmulqdq");
  features.x86_rdrand = detail::bool_from_flag(flags, "rdrand");
  features.x86_rdseed = detail::bool_from_flag(flags, "rdseed");
  features.x86_xsave = detail::bool_from_flag(flags, "xsave");
  features.x86_xsaveopt = detail::bool_from_flag(flags, "xsaveopt");
  features.x86_xsavec = detail::bool_from_flag(flags, "xsavec");

  features.x86_vmx = detail::bool_from_flag(flags, "vmx");
  features.x86_ept = detail::bool_from_flag(flags, "ept");
  features.x86_svm = detail::bool_from_flag(flags, "svm");
  features.x86_npt = detail::bool_from_flag(flags, "npt");

  features.x86_tsc = detail::bool_from_flag(flags, "tsc");
  features.x86_constant_tsc = detail::bool_from_flag(flags, "constant_tsc");
  features.x86_nonstop_tsc = detail::bool_from_flag(flags, "nonstop_tsc");
  features.x86_rdtscp = detail::bool_from_flag(flags, "rdtscp");

  features.arm_neon = any_flag(flags, {"neon", "asimd"});
  features.arm_asimd = detail::bool_from_flag(flags, "asimd");
  features.arm_sve = detail::bool_from_flag(flags, "sve");
  features.arm_sve2 = detail::bool_from_flag(flags, "sve2");
  features.arm_aes = detail::bool_from_flag(flags, "aes");
  features.arm_sha1 = detail::bool_from_flag(flags, "sha1");
  features.arm_sha2 = detail::bool_from_flag(flags, "sha2");
  features.arm_crc32 = detail::bool_from_flag(flags, "crc32");
  features.arm_pauth = any_flag(flags, {"pauth", "paca", "pacg"});

  features.riscv_i = BoolField::value(riscv_has_extension(raw, 'i'));
  features.riscv_m = BoolField::value(riscv_has_extension(raw, 'm'));
  features.riscv_a = BoolField::value(riscv_has_extension(raw, 'a'));
  features.riscv_f = BoolField::value(riscv_has_extension(raw, 'f'));
  features.riscv_d = BoolField::value(riscv_has_extension(raw, 'd'));
  features.riscv_c = BoolField::value(riscv_has_extension(raw, 'c'));
  features.riscv_v = BoolField::value(riscv_has_extension(raw, 'v'));
  features.riscv_h = BoolField::value(riscv_has_extension(raw, 'h'));

  return features;
}

IsaFeatures collect_isa() { return Collector{}.collect_isa(); }

} // namespace archspec
