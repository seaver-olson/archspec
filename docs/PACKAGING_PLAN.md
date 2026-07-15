# Packaging and release plan

The build and install foundation now exists, but public packaging should be
staged. The proposed distribution and executable name is `archspec-inspect` to
avoid collision with the established Python `archspec` package. The C++
namespace and CMake target remain `archspec` and `archspec::archspec`.

## Phase 0: make a release legally and technically possible

- Choose an OSI-approved license and add `LICENSE` plus SPDX headers or a clear
  repository-level declaration. This is the only hard external decision.
- Confirm the product name. If the repository is renamed, preserve redirects
  and keep the JSON producer name stable.
- Add Linux fixture trees for x86-64, AArch64, RISC-V, containers, restricted
  permissions, missing sysfs, and malformed files. Restore accuracy coverage as
  tracked source rather than relying on generated binaries.
- Generate an exhaustive JSON Schema from one field/category definition source.
- Define compatibility: semantic versioning for the library, independent major
  versioning for report schemas, and additive-only changes within schema v1.
- Decide whether the public C++ ABI is stable. Until then, build consumers from
  source or promise only API compatibility within a minor series.

Exit criterion: clean CI on Linux, macOS, and Windows; sanitizer-clean Linux
tests; explicit license; fixture coverage; schema validation; no compiler
warnings.

## Phase 1: publish portable command artifacts

- Tag `v0.2.0` and build release artifacts from CI, not developer machines.
- Produce Linux x86-64 and AArch64 archives, macOS universal or per-architecture
  archives, and Windows x64 ZIPs. Prefer a static C++ runtime where legally and
  technically appropriate, but test compatibility on old supported systems.
- Publish SHA-256 checksums, an SPDX or CycloneDX SBOM, build provenance, and
  signatures. Attach the report schema and example output to each release.
- Exercise `cmake --install` in CI and compile a separate downstream consumer
  through both `find_package(archspec CONFIG)` and `pkg-config archspec`.
- Add a container image only if container execution is a real use case; host
  introspection from a container requires documented `/proc`, `/sys`, and device
  mounts and can otherwise produce misleading results.

Exit criterion: a user can download one archive, run one binary without extra
dependencies, and verify its checksum and report schema.

## Phase 2: package-manager distribution

Start with channels that can consume upstream archives:

1. Homebrew formula for macOS and Linux.
2. Scoop and WinGet manifests for Windows.
3. AUR package as an early Linux packaging feedback loop.
4. `.deb` and `.rpm` packages generated in CI, followed by Debian/Fedora review
   only after API and license maturity.
5. vcpkg or Conan recipes for C++ library consumers.

Use package smoke tests that run `archspec-inspect --capabilities`, validate a
small redacted JSON report, and compile a minimal linked program.

## Phase 3: universal SDK access

The CLI already gives every language a stable process boundary. For in-process
use, add a narrow C ABI before language-specific wrappers:

```c
int archspec_collect_json(const archspec_options*, char** output, size_t* size);
void archspec_free(void*);
```

Keep ownership and error rules explicit, hide C++ types, and version the ABI.
Then build thin Python, Rust, Go, and Node wrappers from the C ABI. Do not expose
the C++ object layout directly through FFI.

## Phase 4: native backend depth

- Separate platform discovery behind internal backend interfaces.
- Add macOS collectors using `sysctl`, IOKit, and SystemConfiguration where
  available.
- Add Windows collectors using documented Win32, SetupAPI, IP Helper, power,
  and performance APIs.
- Evaluate optional hwloc integration for topology rather than reimplementing
  years of platform-specific locality work.
- Add FreeBSD only with a native test host and fixtures; graceful compilation
  without collection depth is not enough to advertise support.

## Release checklist

- [ ] License and third-party notices are present.
- [ ] Version constants, git tag, CMake version, schema version, and changelog agree.
- [ ] Tests, sanitizers, schema validation, downstream install tests, and package smoke tests pass.
- [ ] Artifacts include checksums, SBOM, provenance, and signatures.
- [ ] Documentation states platform depth and privacy behavior accurately.
- [ ] Breaking changes are called out and paired with a migration note.
