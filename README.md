# KV-Engine (Mini-Redis CLI)

A high-performance, in-memory Key-Value Database Engine built from scratch in **C++17**, featuring an **$O(1)$ Least Recently Used (LRU) Cache**, automatic disk persistence, and an interactive Redis-style CLI.

## Key Features
- **$O(1)$ Time Complexity:** Combines `std::unordered_map` and doubly linked list (`std::list`) using iterator splicing (`std::list::splice`) for constant-time `SET`, `GET`, and `DEL` operations.
- **Automatic LRU Eviction:** Evicts the least recently accessed key automatically when memory capacity is reached.
- **Disk Persistence:** Automatically serializes cache state to disk (`kv_data.db`) on shutdown and restores MRU-to-LRU order on startup.
- **Real-Time Telemetry:** Tracks cache hits, cache misses, eviction counts, and hit ratio percentages via the `STATS` command.

## Build & Run Instructions
```bash
mkdir -p build && cd build
cmake ..
make
./kv_engine 5
```

## Supported CLI Commands
- `SET <key> <value>` : Insert or update a key-value pair
- `GET <key>` : Fetch the value of a key in O(1) time
- `DEL <key>` : Remove a key from the store
- `LIST` : Display all records ordered from Most to Least Recently Used
- `STATS` : Show storage usage, hits, misses, evictions, and hit ratio
- `SAVE` : Manually persist in-memory data to disk
- `CLEAR` : Flush all keys from memory
- `EXIT` : Save data to disk and shut down cleanly
