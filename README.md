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
- **Lock-Striped Concurrency (`ShardedKVStore`):** Thread-safe `KVStore` baseline plus a 16-shard lock-striped engine with 64-byte cache-line alignment (`alignas(64)`) to prevent false sharing across CPU cores.
- **Order-Preserving Disk Persistence:** Restores exact MRU-to-LRU cache order across restarts (`data.txt`).
- **Telemetry & Metrics:** Live cache hits, misses, active keys, and hit ratio via `STATS`.
- **Verified Memory & Thread Safety:** 18 automated unit tests (`CTest`) verified in GitHub Actions CI under both AddressSanitizer/UBSan (`-fsanitize=address,undefined`) and ThreadSanitizer (`-fsanitize=thread`).

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

## Running Unit Tests (ASan/UBSan & ThreadSanitizer)

```bash
# Memory & Undefined Behavior Check (ASan + UBSan)
cmake -B build-asan -DENABLE_ASAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure

# Data Race & Concurrency Check (TSan)
cmake -B build-tsan -DENABLE_TSAN=ON
cmake --build build-tsan
ctest --test-dir build-tsan --output-on-failure
```

## Benchmarks
Measured on WSL2 Ubuntu (`g++ 15.2.0`, `-O3` Release build) via `./build-release/kv_bench`:

| Operation | Count | Total Time | Throughput | Avg Latency |
| :--- | :---: | :---: | :---: | :---: |
| SET (in-memory) | 100,000 | 0.1951 s | 512,509 ops/s | 1.95 us |
| GET (in-memory hit) | 100,000 | 0.0799 s | 1,251,962 ops/s | 0.80 us |
| DEL (in-memory) | 100,000 | 0.1129 s | 886,114 ops/s | 1.13 us |
| Snapshot Flush (1K) | 1,000 | 0.0011 s | 904,153 ops/s | 1.11 us |
| 8T Mixed (1-Mutex) | 200,000 | 0.4628 s | 432,148 ops/s | 2.31 us |
| **8T Mixed (16-Shards)** | **200,000** | **0.1484 s** | **1,347,773 ops/s** | **0.74 us** |

> Lock striping across 16 cache-aligned shards delivers a **3.12x concurrent throughput speedup** and **~68% lower average latency** compared to a single global mutex under an 8-thread mixed (80% GET / 20% SET) workload.

## Roadmap (Planned for v2.0)
- **Append-Only WAL & Compaction:** Replace full-snapshot rewrites on interactive mutations with crash-safe write-ahead logging.
- **POSIX TCP Server:** Expose the sharded engine over a lightweight localhost client-server protocol.
