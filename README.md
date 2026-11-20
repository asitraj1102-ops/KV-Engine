# KV-Engine: C++17 In-Memory Key-Value Store with LRU Cache

![C++17 CI](https://github.com/asitraj1102-ops/KV-Engine/actions/workflows/ci.yml/badge.svg)
![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build](https://img.shields.io/badge/Build-CMake-informational.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

A lightweight, persistent command-line Key-Value storage engine written in **C++17**. It implements an **$O(1)$ Least Recently Used (LRU) Cache** eviction policy using `std::unordered_map` and `std::list` (Doubly Linked List), along with order-preserving disk persistence, real-time cache telemetry (`STATS`), 15 automated unit tests, and throughput benchmarks.

## Architecture

```text
  CLI Shell (SET / GET / DEL / LIST / STATS)
                     |
                     v
  +--------------------------------------+
  |            KVStore Engine            |
  |  +--------------------------------+  |
  |  | std::unordered_map (O(1) key)  |--+--> std::list (MRU -> LRU order)
  |  +--------------------------------+  |
  |                  |                   |
  |                  v                   |
  |      Snapshot Disk Persistence       |---> data.txt (Exact order reload)
  +--------------------------------------+
```

## Key Features
- **O(1) Cache Operations:** Hash map + doubly linked list iterators for constant-time `SET`, `GET`, `DEL`, and LRU eviction.
- **Order-Preserving Disk Persistence:** Restores exact MRU-to-LRU cache order across restarts (`data.txt`).
- **Telemetry & Metrics:** Live cache hits, misses, active keys, and hit ratio via `STATS`.
- **Verified Memory Safety:** 15 automated unit tests (`CTest`) verified with AddressSanitizer & UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`).

## How to Build & Run (Linux / WSL2)

```bash
git clone https://github.com/asitraj1102-ops/KV-Engine.git
cd KV-Engine

# Release Build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# Launch Interactive Shell
./build-release/kv_engine
```

### Sample CLI Usage
```text
kv-engine> SET lang C++17 Systems
[OK] Saved lang = C++17 Systems
kv-engine> GET lang
"C++17 Systems"
kv-engine> STATS
kv-engine> EXIT
```

## Running Unit Tests (with AddressSanitizer)

```bash
cmake -B build-asan -DENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

## Benchmarks
Measured on WSL2 Ubuntu (`g++ 15.2.0`, `-O3` Release build) via `./build-release/kv_bench`:

| Operation | Count | Total Time | Throughput | Avg Latency |
| :--- | :---: | :---: | :---: | :---: |
| SET (in-memory) | 50,000 | 0.0261 s | 1,915,192 ops/s | 0.52 us |
| GET (in-memory hit) | 50,000 | 0.0113 s | 4,414,688 ops/s | 0.23 us |
| DEL (in-memory) | 50,000 | 0.0220 s | 2,275,773 ops/s | 0.44 us |
| Snapshot Flush (1K) | 1,000 | 0.0003 s | 3,842,991 ops/s | 0.26 us |

## Known Limitations (v1.0.0)
- Single-threaded local execution (`std::shared_mutex` concurrency and TCP client-server protocol planned for v2.0).
- Full snapshot rewrite on interactive mutations (append-only WAL planned for v2.0).
