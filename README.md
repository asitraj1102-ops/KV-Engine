# KV-Engine v2.0 (Mini-Redis CLI)

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C?style=for-the-badge&logo=cmake&logoColor=white)
![Linux](https://img.shields.io/badge/Platform-Linux%20(WSL2)-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Time Complexity](https://img.shields.io/badge/Lookup%20%26%20Eviction-O(1)-brightgreen?style=for-the-badge)

A high-performance, in-memory **Key-Value Database Engine** built from scratch in **C++17**. Inspired by **Redis**, it solves slow disk-read bottlenecks by storing active records in RAM with **$O(1)$ lookups**, **automatic Least Recently Used (LRU) eviction**, **TTL key expiration**, and **automated disk persistence**.

---

## 💡 What Problem Does It Solve? (Real-World Use Case)
Traditional databases (like MySQL) read/write from disk, which takes milliseconds. High-traffic systems (Instagram feeds, OTP verification services, Amazon shopping carts) need **microsecond** responses.
- **Instant Caching:** `KV-Engine` keeps hot data in RAM using an $O(1)$ Hash Map + Doubly Linked List architecture.
- **OTP & Session Expiry (`EXPIRE` / `TTL`):** Automatically deletes temporary keys (like 60-second OTPs) once their timer expires.
- **Bounded Memory Safety (LRU):** Never crashes when RAM is full—automatically evicts the least recently accessed key in $O(1)$ time.

---

## 🏗️ System Architecture

```text
+-----------------------------------------------------------------------+
|                       Interactive CLI Parser                          |
|     (SET, GET, DEL, EXPIRE, TTL, LIST, STATS, BENCHMARK, SAVE)        |
+-----------------------------------+-----------------------------------+
                                    |
                                    v
+-----------------------------------------------------------------------+
|                     KV-Engine Core (In-Memory RAM)                    |
|                                                                       |
|  1. Fast O(1) Index (std::unordered_map<string, ListIterator>)        |
|     [ "user4" ] ------------------------+                             |
|     [ "user3" ] ---+                    |                             |
|                    |                    |                             |
|                    v                    v                             |
|  2. Usage Order & TTL Tracker (Doubly Linked List - std::list)        |
|     [MRU / Front] <--> [user3: "Priya"] <--> [user4: "Vikram"] [LRU]  |
+-----------------------------------+-----------------------------------+
                                    |
                     Auto-Save on Shutdown / Load on Startup
                                    v
+-----------------------------------------------------------------------+
|                   Disk Persistence Layer (kv_data.db)                 |
+-----------------------------------------------------------------------+
```

---

## 🖥️ Live Terminal Demo (How It Works)

```text
========================================================
   KV-Engine v2.0 (Mini-Redis CLI) | C++17 Storage      
   Features: O(1) LRU Cache | TTL Expiry | Disk Engine  
   Max Cache Capacity: 3 keys (Type HELP for commands)
========================================================
kv-engine> SET otp_101 849201
OK (inserted)
kv-engine> EXPIRE otp_101 10
(integer) 1 (expires in 10s)
kv-engine> TTL otp_101
(integer) 8s remaining

kv-engine> SET user1 Aman
OK (inserted)
kv-engine> SET user2 Rahul
OK (inserted)
kv-engine> SET user3 Priya
[LRU Eviction] Capacity full! Removed least recently used key: 'otp_101'
OK (inserted)

kv-engine> BENCHMARK
[Benchmark] Running 100000 O(1) SET + GET operations in RAM...
----------------------------------------------------
Total Operations : 200000 (100000 SETs + 100000 GETs)
Time Taken       : 42.18 ms
Throughput       : ~4741580 operations/sec
----------------------------------------------------
```

---

## ⚙️ Supported CLI Commands
| Command | Description | Time Complexity |
| :--- | :--- | :--- |
| `SET <key> <value>` | Insert or update a key-value pair | $O(1)$ |
| `GET <key>` | Fetch the value of a key | $O(1)$ |
| `DEL <key>` | Remove a key from the store | $O(1)$ |
| `EXPIRE <key> <sec>` | Set automatic expiration timer (TTL) on a key | $O(1)$ |
| `TTL <key>` | Check remaining seconds before key auto-deletes | $O(1)$ |
| `LIST` | Display all active records (MRU to LRU) | $O(N)$ |
| `STATS` | Show storage usage, hits, misses & hit ratio | $O(1)$ |
| `BENCHMARK` | Run 200,000 live operations throughput test | $O(N)$ |
| `SAVE` / `EXIT` | Persist in-memory data to disk (`kv_data.db`) | $O(N)$ |

## 🚀 Build & Run Instructions
```bash
mkdir -p build && cd build
cmake ..
make
./kv_engine 5
```
