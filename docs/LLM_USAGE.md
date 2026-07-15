# LLM and agent usage

The supported integration is a subprocess with JSON on stdout. It avoids an SDK
dependency in the agent's language and works in shells, Python, Rust, Go, Node,
and tool-execution sandboxes.

Discover the contract without collecting system data:

```sh
archspec-inspect --capabilities
```

Request only the context needed for a task:

```sh
archspec-inspect --format json --categories os,cpu,isa,memory --redact
```

Operational rules for callers:

1. Require exit code `0` before parsing stdout. CLI usage errors return `2` and
   diagnostics are written only to stderr.
2. Check `schema_version` before consuming `data`.
3. Check each leaf's `valid` flag. If false, use `status` instead of guessing a
   value or treating every failure as zero.
4. Prefer the smallest category list. This reduces latency, tokens, and exposed
   host data.
5. Keep redaction enabled for reports that may leave the machine. The CLI
   defaults to redaction; `--include-sensitive` is an explicit opt-in.
6. Do not enable active probes unless their result is required. In particular,
   `--allow-perf` makes system calls that may be blocked by sandbox policy.

Minimal consumer logic in pseudocode:

```text
result = run(["archspec-inspect", "--format", "json",
              "--categories", "cpu,memory", "--redact"])
assert result.exit_code == 0
report = parse_json(result.stdout)
assert report.schema_version == "1.0.0"
if report.data.cpu.model_name.valid:
    use(report.data.cpu.model_name.value)
else:
    explain(report.data.cpu.model_name.status)
```

An MCP server is not required for the first release: it would wrap a single
deterministic command while adding another runtime and protocol surface. Add one
later only if clients need remote collection, policy enforcement, caching, or
multiple operations such as diffing and querying saved reports.
