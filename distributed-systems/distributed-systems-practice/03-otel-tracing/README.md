# OpenTelemetry distributed tracing on Papy

This instruments [Papy](https://github.com/noahpop77/Papy) — a multi-threaded
C++ HTTP load-testing CLI — with the real OpenTelemetry C++ SDK, then proves
the tracing is genuinely *distributed* (not just nested logging inside one
process) by running a second, separate program — a tiny demo HTTP server —
that picks up Papy's trace via the real W3C `traceparent` header and
continues it in its own spans.

Two binaries come out of this:

- **Papy itself** (`papy-src/Papy-main`), with tracing added at every request
  site.
- **`target_server`** (`server/`), a throwaway HTTP target for Papy to hit,
  built to show the other half of a distributed trace — not something you'd
  ship.

See `ANALYSIS.md` for exactly what's real here (the SDK, the spans, the
propagation) versus simplified (the exporter), and for a pre-existing Papy
build defect that had to be fixed along the way to get any of this running
on Linux at all.

## What's here

```
03-otel-tracing/
├── papy-src/Papy-main/      Papy's own source, instrumented in place
│   ├── src/otelTracing.{hpp,cpp}     SDK setup (TracerProvider, exporter)
│   ├── src/httpPropagation.hpp       W3C header carrier for cpp-httplib
│   ├── src/threadWorks.cpp           where the actual spans are created
│   ├── src/apiClient.{hpp,cpp}       threaded extra request headers through
│   └── makefile                      builds papy with OTel linked in
├── server/                  the demo target server
│   ├── target_server.cpp
│   ├── otelTracing.{hpp,cpp}, httpPropagation.hpp  (own copies — see below)
│   └── makefile
├── third_party/otel-cpp/    pre-built OpenTelemetry C++ SDK (include/ + lib/)
└── scripts/
    ├── setup_otel.sh        rebuilds third_party/otel-cpp from source
    └── demo.sh              runs both binaries and shows the matched trace
```

`server/` has its own copies of `otelTracing.{hpp,cpp}` and
`httpPropagation.hpp` rather than sharing Papy's. That's deliberate: in a
real deployment the client and the target are two unrelated codebases that
happen to agree on the OTel API and the W3C header format, not two programs
sharing a source tree. Duplicating ~100 lines of glue once is cheaper than
pretending otherwise.

## Setup

`third_party/otel-cpp/` already ships a working, pre-built copy (headers +
the 6 static libraries actually needed), so **you can build immediately
without running anything else.** If you want to rebuild it from source
yourself — to verify where it came from, bump the OTel version, or just see
the recipe — run:

```
./scripts/setup_otel.sh
```

This clones `opentelemetry-cpp`, configures it with OTLP/gRPC/HTTP, Zipkin,
Prometheus and Elasticsearch export all disabled (none of that is needed for
real distributed tracing with W3C propagation — see ANALYSIS.md for why),
builds the trace API/SDK core and the console exporter, and copies the
result into `third_party/otel-cpp/`. It needs `git`, `cmake`, a C++17
compiler, and network access to github.com. It takes under a minute on a
2-core machine.

## Build

```
make -C papy-src/Papy-main      # -> papy-src/Papy-main/bin/papy
make -C server                  # -> server/bin/target_server
```

Both link the same prebuilt `third_party/otel-cpp` libraries. Papy also
needs its own vendored OpenSSL/zlib, which ship inside the repo already
(`papy-src/Papy-main/src/dependencies/`) — see ANALYSIS.md if you're
rebuilding on a platform other than the one this was developed on.

## Run the demo

```
./scripts/demo.sh            # picks port 8099
./scripts/demo.sh 9000       # or your own port
```

This starts `target_server`, points Papy at it for one request, prints both
processes' spans, and checks that they share a `trace_id`:

```
================= Verdict ===============================
Client trace_id: cae3f40157d5cd3976900ad3de9d5bad
Server trace_id: cae3f40157d5cd3976900ad3de9d5bad
MATCH — one trace, two processes. This is what makes it distributed
tracing and not just structured logging inside Papy.
```

### Running it by hand

```
# terminal 1
./server/bin/target_server --port 8099 --log /tmp/server-trace.log

# terminal 2
PAPY_TRACE_LOG=/tmp/client-trace.log \
  ./papy-src/Papy-main/bin/papy --target "http://127.0.0.1:8099" \
    --endpoint "/printJson" --count 1 --verbose
```

`PAPY_TRACE_LOG` (optional) sends Papy's spans to a file instead of stdout —
useful since Papy's own live stats line is also writing to stdout.
`target_server --log` does the same on the server side. Both default to
stdout if unset.

Works with every one of Papy's existing payload modes — `--payload lol`,
`--payload ocean`, `--payload payload.json`, or a path to your own JSON file
— and with `--threads`/`--count` for concurrent load. Each request gets its
own trace; `target_server` doesn't care what the endpoint or payload is.

## A note on the sandbox this was built in

Same constraint as Project 2: this sandbox's package managers are
effectively unusable for anything beyond pure-Python/pure-JS packages (apt
returns 403 from every Ubuntu mirror; pip has no `grpcio` wheel for this
platform; npm 403s on literally everything, including `lodash`). OTLP
export needs gRPC or a libcurl-backed HTTP client, neither of which could be
installed here — so the SDK was configured with OTLP disabled, and spans go
to an `OStreamSpanExporter` (structured text, to a file or stdout) instead
of a real collector. Everything upstream of the exporter — the SDK, the
spans, the context propagation across the two processes — is the real
thing; see ANALYSIS.md for exactly where the line is and what it would take
to move it.
