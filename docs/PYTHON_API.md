# Python API

`archspec_inspect` is a small CPython extension over the C++17 library. It is
not the unrelated CPU-microarchitecture package named `archspec`.

The collector and JSON serialization run in native C++ and release the GIL
while collecting. Use `collect_json()` when JSON is your desired result: it
returns the C++ serialization directly and avoids Python object conversion.
`collect()` is the convenient version; it decodes that same report into a
standard Python dictionary.

## Build and import

The binding is enabled by default when CMake can find Python development
headers. It can be disabled with `-DARCHSPEC_BUILD_PYTHON=OFF`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
PYTHONPATH=build/python python3 -c 'import archspec_inspect; print(archspec_inspect.version())'
```

For installation, set `ARCHSPEC_PYTHON_INSTALL_DIR` to the package directory
for the target interpreter before running `cmake --install`. For example:

```sh
cmake -S . -B build \
  -DARCHSPEC_PYTHON_INSTALL_DIR="$(python3 -c 'import sysconfig; print(sysconfig.get_path("platlib"))')/archspec_inspect"
cmake --build build
cmake --install build
```

## `collect_json`

```python
import archspec_inspect

report_json = archspec_inspect.collect_json(
    ["cpu", "isa", "memory"],
    include_sensitive=False,
)
```

`collect_json(categories=None, *, include_sensitive=False,
allow_perf_open=False, allow_slow_probes=False,
allow_vendor_libraries=False, procfs_root=None, sysfs_root=None,
etc_root=None, dev_root=None) -> str`

`categories` is one of:

- `None` for every category;
- a comma-separated string such as `"cpu,isa"`; or
- an iterable of category names such as `("cpu", "isa")`.

The available names are `os`, `cpu`, `isa`, `cache`, `memory`, `pci`, `gpu`,
`perf`, `block`, `net`, `thermal`, `power`, `virtualization`, and `platform`.
The special name `all` selects every category. Unknown or empty category lists
raise `ValueError`.

Sensitive host information is redacted by default: hostname, kernel command
line, MAC addresses, and IP addresses. Set `include_sensitive=True` only when
that data is needed and will remain local. The probe options are off by default
and directly match their C++ counterparts. `allow_perf_open=True` permits
`perf_event_open` capability probes; the remaining two are reserved opt-in
controls for slow or vendor-library probes.

The four root arguments provide captured or fixture trees, mirroring
`CollectOptions` in C++:

```python
fixture_report = archspec_inspect.collect_json(
    "cpu,memory",
    procfs_root="tests/fixture/proc",
    sysfs_root="tests/fixture/sys",
    etc_root="tests/fixture/etc",
    dev_root="tests/fixture/dev",
)
```

When an alternate `sysfs_root` is supplied, network addresses are intentionally
not read from the running host. This keeps offline reports self-contained.

## `collect`

```python
report = archspec_inspect.collect(["os", "cpu"])
cpu = report["data"]["cpu"]

model = cpu["model_name"]
if model["valid"]:
    print(model["value"])
else:
    print(model["status"])
```

`collect()` has the same arguments as `collect_json()` and returns the decoded
report dictionary. Each scalar has a `valid` flag and a stable `status`; a
`value` key is included only when `valid` is true. The report envelope and
category payloads conform to the [report schema](../schema/archspec-report-v1.schema.json).

`version()` returns the version string of the linked native library.
