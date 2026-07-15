# Competitive assessment

Assessment date: 2026-07-03.

ArchSpec Inspect is currently strongest as a small, embeddable, status-aware
Linux snapshot library. It is not yet a replacement for mature topology tools,
fleet query engines, or cross-platform presentation tools.

| Tool | Primary job | Platforms | Interfaces | Relative position |
|---|---|---|---|---|
| ArchSpec Inspect | Broad local hardware/OS snapshot | Deep on Linux; basic fallback on macOS and Windows | C++17 and versioned JSON CLI | Small and dependency-free; uniquely explicit per-field failure status and fixture roots, but immature |
| [hwloc](https://www.open-mpi.org/projects/hwloc/) | Hardware topology, locality, and CPU/memory binding | Linux, BSDs, macOS, Windows, Solaris, AIX, Android, and more | Mature C API and CLI/export tools | Far deeper and more portable for topology; ArchSpec Inspect covers more operational categories in one simple report |
| [lshw](https://ezix.org/src/pkg/lshw) | Detailed Linux hardware inventory tree | Linux across several CPU families | CLI with text, XML, and HTML | Deeper device inventory and firmware probing; ArchSpec Inspect has a simpler embeddable API and explicit missing-data semantics |
| [osquery](https://osquery.io/) | SQL access to endpoint state and fleet operations | Linux, macOS, Windows | SQL tables, daemon, extensions | Much broader and production-proven, but substantially larger; ArchSpec Inspect is suited to one-shot local snapshots and embedding |
| [Fastfetch](https://github.com/fastfetch-cli/fastfetch) | Fast human-readable system summary | Linux, macOS, Windows, Android, BSDs, and others | Highly configurable CLI and JSON | Far more polished and portable for presentation; ArchSpec Inspect aims at stable programmatic fields and diagnostics rather than terminal presentation |
| [archspec (Python)](https://github.com/archspec/archspec) | Detect, label, compare, and reason about CPU microarchitectures | Python-supported hosts | Python API and CLI | Complementary rather than competing; its established package name is the reason this project should ship as `archspec-inspect` |

## What is already differentiated

- A field is not merely `null`: it states whether data was unsupported, absent,
  denied, malformed, redacted, or failed internally.
- Alternate filesystem roots make Linux collectors testable without running on
  the target host. This is useful for regression fixtures and offline analysis.
- Collection is safe and bounded by default: no shell commands, no network, no
  vendor libraries, and no active performance-counter probes.
- The same typed model serves an embeddable C++ API and a self-describing JSON
  process boundary.

## Gaps that matter

1. Native macOS and Windows collectors are mostly absent. Compiling everywhere
   is not the same as collecting deeply everywhere.
2. CPU/cache/NUMA topology is much less rigorous than hwloc and is not aware of
   cgroup/cpuset restrictions, hybrid core kinds, CXL, or accelerator locality.
3. Device names are often raw PCI IDs; there is no maintained ID database or
   optional enrichment layer.
4. The schema currently fixes the report envelope and status vocabulary but
   does not constrain every category property. A generated, exhaustive schema
   should precede a 1.0 format promise.
5. There are no captured-system fixture suites, fuzz tests, benchmarks, ABI
   policy, signed releases, or downstream packages yet.
6. The repository has no explicit license, which blocks responsible reuse and
   most package-manager submissions.

## Product direction

Do not compete with hwloc on topology algorithms or osquery on fleet
management. The defensible niche is a small, safe snapshot component with
excellent missing-data semantics and a stable language-neutral protocol. Use
optional backends or enrichment sources where mature projects already solve a
hard platform problem.
