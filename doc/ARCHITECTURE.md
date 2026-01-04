# TitanServer Architecture Notes

## Why no frameworks?
Frameworks hide complexity. This project exists to expose it.

## Why manual HTTP parsing?
Understanding HTTP parsing is critical to understanding:
- performance bottlenecks
- security issues
- protocol correctness

## Why an LRU cache?
Disk I/O is orders of magnitude slower than memory.
Caching frequently accessed files dramatically reduces latency.

## Why separate parsing, routing, and networking?
This mirrors real server architectures and makes each component testable,
replaceable, and extensible.

## Why start single-threaded?
Correctness before concurrency. Thread pools are added only after the
core logic is proven correct.
