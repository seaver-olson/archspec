"""Native Python interface for ArchSpec Inspect."""

from __future__ import annotations

import json
from typing import Any, Iterable, Mapping, Optional, Union

from ._native import collect_json, version

Categories = Optional[Union[str, Iterable[str]]]


def collect(
    categories: Categories = None,
    *,
    include_sensitive: bool = False,
    allow_perf_open: bool = False,
    allow_slow_probes: bool = False,
    allow_vendor_libraries: bool = False,
    procfs_root: Optional[str] = None,
    sysfs_root: Optional[str] = None,
    etc_root: Optional[str] = None,
    dev_root: Optional[str] = None,
) -> Mapping[str, Any]:
    """Collect a report and decode the native JSON into a Python mapping.

    ``categories`` is ``None`` (all categories), a comma-separated string, or
    an iterable such as ``["cpu", "memory"]``. Sensitive fields are redacted
    unless explicitly enabled.
    """
    return json.loads(
        collect_json(
            categories,
            include_sensitive=include_sensitive,
            allow_perf_open=allow_perf_open,
            allow_slow_probes=allow_slow_probes,
            allow_vendor_libraries=allow_vendor_libraries,
            procfs_root=procfs_root,
            sysfs_root=sysfs_root,
            etc_root=etc_root,
            dev_root=dev_root,
        )
    )


__all__ = ["collect", "collect_json", "version"]
