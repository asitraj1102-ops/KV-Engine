# KV-Engine (Mini-Redis CLI)

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
![Linux](https://img.shields.io/badge/Platform-Linux%20(WSL2)-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Time Complexity](https://img.shields.io/badge/Lookup%20%26%20Eviction-O(1)-brightgreen?style=for-the-badge)

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
| Command | Description | Time Complexity |
| :--- | :--- | :--- |
| `SET <key> <value>` | Insert or update a key-value pair | $O(1)$ |
| `GET <key>` | Fetch the value of a key | $O(1)$ |
| `DEL <key>` | Remove a key from the store | $O(1)$ |
| `LIST` | Display all records (MRU to LRU) | $O(N)$ |
| `STATS` | Show storage usage, hits, misses & hit ratio | $O(1)$ |
| `SAVE` | Manually persist in-memory data to disk | $O(N)$ |
| `CLEAR` | Flush all keys from memory | $O(N)$ |
| `EXIT` | Save data to disk and shut down cleanly | $O(N)$ |
