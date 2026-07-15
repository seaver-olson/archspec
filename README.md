# ArchSpec Inspect

ArchSpec Inspect is a small C++17 library and command-line tool for describing
the machine it is running on. It collects structured information about the OS,
CPU topology and instruction set, caches, memory and NUMA, PCI and graphics,
storage, networking, thermal and power state, virtualization, and firmware.

The project is Linux-first. On other platforms, the portable parts still work
and unsupported information is reported explicitly rather than guessed.

> This project is unrelated to the Python package named
> [`archspec`](https://github.com/archspec/archspec), which models CPU
> microarchitectures. The executable and Python package here use the
> `archspec-inspect` and `archspec_inspect` names to avoid that collision.

## Build it

The C++ library has no runtime dependencies. CMake is the supported build and
installation path:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

`make -j` and `make test` are also available for a quick local build.

## Command line

```sh
./build/archspec-inspect
./build/archspec-inspect --format json --categories cpu,isa,memory
./build/archspec-inspect --capabilities
```

The CLI redacts hostname, kernel command line, MAC addresses, and IP addresses
by default. Add `--include-sensitive` only when the report will stay local.
It is read-only, does not invoke shell commands, and does not use the network.

## Use it from C++

After installation, add the library to a CMake target:

```cmake
find_package(archspec CONFIG REQUIRED)
target_link_libraries(my_program PRIVATE archspec::archspec)
```

Then collect only the information your program needs:

```cpp
#include <archspec/archspec.hpp>

#include <iostream>

int main() {
  archspec::CollectOptions options;
  options.categories =
      archspec::CollectCategory::cpu |
      archspec::CollectCategory::memory;
  options.include_sensitive = false;

  const archspec::Report report = archspec::collect_report(options);
  std::cout << archspec::to_json(report) << '\n';
}
```

Every scalar in the report carries a status. A field is either valid and has a
value, or it explains why no value is available (`not_found`, `unsupported`,
`perm_denied`, `parse_error`, or `redacted`). The original `#include
<aview.hh>` remains available for source compatibility.

## Use it from Python

When Python development headers are installed, the CMake build also produces
the native `archspec_inspect` extension:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
PYTHONPATH=build/python python3 - <<'PY'
import archspec_inspect

report = archspec_inspect.collect(["cpu", "isa"])
print(report["data"]["cpu"]["model_name"])
PY
```

For code that wants JSON, `collect_json()` returns the C++ serializer’s output
directly. `collect()` is the convenient version and returns a normal Python
dictionary. Both collectors run natively and release the GIL while gathering
the report.

```python
import archspec_inspect

payload = archspec_inspect.collect_json("cpu,memory")
report = archspec_inspect.collect(["os", "cpu"], include_sensitive=False)
```

The full API, installation instructions, fixture roots, and privacy controls
are in the [Python API guide](docs/PYTHON_API.md).

## Reports and project notes

JSON reports include the producer and schema versions, the categories that
were collected, and only the selected data. The report format is defined in
[schema/archspec-report-v1.schema.json](schema/archspec-report-v1.schema.json).

- [Comparison with other introspection tools](docs/COMPARISON.md)
- [Packaging and release plan](docs/PACKAGING_PLAN.md)
- [Using the CLI from LLM tools and agents](docs/LLM_USAGE.md)

The project needs an explicit open-source license before it can be published
to package repositories.
