# TitanServer

A multithreaded HTTP/1.1 server written from scratch in C++17 — no frameworks, no libraries beyond the standard library and Winsock.

The interesting part of this project is not that it serves files. It's what happened when I load-tested it: **the thread pool I was proud of turned out to be irrelevant, and the two real bottlenecks were things I had never thought about.** This README is mostly about that.

---

## Performance

All figures: 8-core Windows machine, x64 Release build (MSVC `/O2`), `autocannon -c 8 -d 15`, median of 3 runs.

| Stage | Throughput | p99 latency | Errors |
|---|---|---|---|
| Baseline | 3,084 req/s | 6,691 ms | ~50% |
| Graceful TCP close | 3,084 req/s | 6,691 ms | **0** |
| **+ HTTP keep-alive** | **42,558 req/s** | **15 ms** | 0 |
| **+ Log level gating** | **59,182 req/s** | **16 ms** | 0 |

**Net: 13.8× from keep-alive, a further 11.5× from log gating.** Zero errors across 228k requests.

> **Honest caveat:** the load generator (autocannon, Node) runs on the same 8 cores as the server. At ~59k req/s both `/health` and `/index.html` converge on the *same* number, which strongly suggests the client is now the limiter. **59k is a floor, not a ceiling.** Measuring from a separate machine would give a truer figure.

---

## What the benchmarks found

### 1. Half the requests were failing — and it wasn't the application's fault

The first load test reported ~50% errors. The responses were valid; the *connections* were not.

`closesocket()` on a Windows socket that still has **unread inbound data** sends a TCP **RST** instead of a FIN. The client sees a connection reset and reports an error even though it already received a correct response. And there was a guaranteed race: the client wrote its next request into the socket the moment it got a response, so unread data almost always existed.

**Fix:** `shutdown(SD_SEND)`, drain the receive buffer, then close.
**Result:** error rate → 0.

### 2. The server was exhausting the operating system's port space

With errors gone, latency was still absurd: **p99 of 6.7 seconds**, and Little's Law didn't hold — `throughput × latency` implied ~27,000 requests in flight over 50 connections, which is impossible. That impossibility was the clue: the numbers were not measuring server work.

Measured `TIME_WAIT` sockets before and after a 15-second run:

```
before:      2
after:  15,495
```

Windows has ~16,384 ephemeral ports. The server closed the connection after every single request, so a 15-second load test **consumed 95% of the machine's entire port range**. New connections then stalled waiting for ports to be released — which is why latency ramped steadily upward through every run.

**Fix:** implement HTTP keep-alive — loop `recv`/`send` on one socket, honouring the `Connection` header and HTTP/1.1's persistent-by-default semantics, with an idle timeout so a quiet client can't pin a worker forever.

**Result: 3,084 → 42,558 req/s (13.8×). p99 6,691 ms → 15 ms. TIME_WAIT 15,495 → 632.**

The bottleneck was never the code. It was the protocol.

### 3. The bottleneck was `std::cout`

With connection churn fixed, a new gap appeared:

| Endpoint | Req/s | Stdev |
|---|---|---|
| `/health` (no logging) | 41,231 | 12,253 |
| `/index.html` (2 log writes) | 5,138 | **24.6** |

The **standard deviation is the tell.** 24.6 req/s on a mean of 5,138 — a variance of 0.5%. Nothing CPU-bound is that stable. A number that rigid means the system is pinned against a *fixed-rate serial resource*: eight worker threads, all queued on one mutex, all blocking on one flushing `std::cout`.

The server could do 41,000 req/s. The console could do 5,138.

**Fix:** a log level checked with a relaxed atomic load **before** taking the mutex, so a suppressed log costs one atomic read — no lock, no string formatting, no I/O. Demoted per-request logs to `debug`. Replaced `std::endl` (which forces a flush every line) with `"\n"`.

**Result: 5,138 → 59,182 req/s (11.5×).**

Note this bottleneck was completely **invisible** until the first one was fixed — before keep-alive, the same comparison showed only a 20% gap. Fix one thing, re-measure, repeat.

---

## Where the design breaks

The thread pool sizes itself to `hardware_concurrency()` — 8 threads on this machine. With keep-alive, **a worker thread is pinned to a connection for its entire lifetime**, not just one request. So:

**8 threads → 8 concurrent connections.** A 9th client waits in the accept queue.

At `-c 100` the aggregate throughput still looks excellent (59,113 req/s) — but that number hides a catastrophic tail:

```
p50 latency:    0 ms
p99 latency:    1 ms
max latency: 1755 ms      <-- starved connections
stdev:        108 ms      <-- 14x the mean
```

A standard deviation 14× the mean isn't a distribution, it's two distributions glued together: connections holding a thread are served instantly; connections waiting for one stall for seconds. **Reporting only the mean would have hidden this entirely.**

This is the C10K problem, reproduced in miniature. Thread-per-connection does not scale, no matter how good the thread pool is.

**The real fix is event-driven I/O** — IOCP on Windows, `epoll` on Linux — where a handful of threads multiplex thousands of sockets. That is the next thing I would build, and it is why nginx exists.

### Other known limitations

- Windows-only (Winsock). Portability would need an abstraction over the socket layer.
- One `recv()` per request; a request split across TCP segments would be mis-parsed. No request pipelining.
- `LRUCache` takes a global mutex on every lookup — a likely next bottleneck under a workload that actually stresses it (a sharded cache or `shared_mutex` would fix it). It never surfaced here because the console was slower.
- `WEB_ROOT` is a hard-coded path; it should be a command-line argument.
- GET only.

---

## Architecture

```
accept()  ->  thread pool  ->  HttpParser  ->  HttpHandler  ->  HttpResponse
                                                    |
                                              LRUCache (file contents)
                                                    |
                                              FileLoader (disk)
```

| Component | Role |
|---|---|
| `TcpServer` | Accept loop, socket options, keep-alive request loop, graceful close |
| `ThreadPool` | Fixed-size pool, `std::thread` + `condition_variable`, unbounded task queue |
| `HttpParser` | Request line, headers, body |
| `HttpHandler` | Routing, cache lookup, response construction |
| `LRUCache<K,V>` | Thread-safe, O(1) get/put — hash map + intrusive `std::list` |
| `FileLoader` | Disk reads, MIME type resolution |
| `Logger` | Singleton, level-gated, mutex-guarded |

### Routes

| Route | Purpose |
|---|---|
| `/health` | Liveness JSON — no I/O, no logging (used as the benchmark control) |
| `/stats` | Request counter |
| `/slow` | Sleeps 3s — demonstrates worker starvation in a fixed pool |
| `/dashboard` | Browser dashboard |
| `/*` | Static files from `www/`, served through the LRU cache |

---

## Build

Requires CMake 3.20+, MSVC (C++17), Windows SDK.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\TitanServer.exe
```

Then visit `http://localhost:8080/`.

**Build Release for any benchmarking.** A Debug-build number is meaningless.

## Reproducing the benchmarks

```powershell
npm install -g autocannon

autocannon -c 8 -d 5  http://localhost:8080/health      # warm-up, discard
autocannon -c 8 -d 15 http://localhost:8080/health
autocannon -c 8 -d 15 http://localhost:8080/index.html

# Connection churn check
(netstat -an | Select-String "TIME_WAIT").Count

# Find the saturation knee
foreach ($c in 1,2,4,8,16,32,50,100) {
    Write-Host "`n===== connections = $c ====="
    autocannon -c $c -d 10 http://localhost:8080/health 2>&1 |
        Select-String "Req/Sec|Latency|errors"
}
```

**Sanity-check every result against Little's Law** (`concurrency ≈ throughput × latency`). If `req/s × avg latency` doesn't land near your connection count, the benchmark is measuring something other than what you think — that check is what exposed both major bugs here.

---

## What I'd do next

1. **IOCP event loop** — decouple threads from connections, break the C10K ceiling.
2. **Bounded task queue with backpressure** — the accept loop currently accepts work it cannot serve.
3. **Shard the LRU cache** — remove the global mutex before it becomes the bottleneck.
4. **Benchmark from a second machine** — the current ceiling may be the load generator, not the server.

## Design notes

Frameworks hide complexity; this project exists to expose it. Manual HTTP parsing forces you to confront protocol correctness, performance, and security directly. Separating parsing, routing, caching, and networking mirrors real server architecture and keeps each piece testable.

The largest lesson was methodological: **I optimised the wrong thing until I measured.** The thread pool — the part I designed most carefully — was never the bottleneck. A socket close and a `cout` flush were.