#include "KVStore.hpp"
#include "ShardedKVStore.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int main() {
    auto measure = [](const std::string& label, std::size_t count, auto&& fn) {
        auto start = std::chrono::steady_clock::now();
        fn();
        auto end = std::chrono::steady_clock::now();

        double sec = std::chrono::duration<double>(end - start).count();
        double ops = static_cast<double>(count) / sec;
        double avgUs = (sec * 1e6) / static_cast<double>(count);

        std::cout << "| " << std::left << std::setw(24) << label
                  << " | " << std::setw(8) << count
                  << " | " << std::fixed << std::setprecision(4) << sec << "s"
                  << " | " << std::setprecision(0) << ops << " ops/s"
                  << " | " << std::setprecision(2) << avgUs << " us |\n";
    };

    constexpr std::size_t N = 100000;
    KVStore memDb(N, "", false);

    std::cout << "| Operation                | Count    | Total Time | Throughput | Avg Latency |\n";
    std::cout << "| :---                     | :---:    | :---:      | :---:      | :---:       |\n";

    measure("SET (in-memory)", N, [&] {
        for (std::size_t i = 0; i < N; ++i) {
            memDb.set("key_" + std::to_string(i), "val_" + std::to_string(i));
        }
    });

    std::size_t hits = 0;
    measure("GET (in-memory hit)", N, [&] {
        for (std::size_t i = 0; i < N; ++i) {
            if (memDb.get("key_" + std::to_string(i)).has_value()) hits++;
        }
    });

    measure("DEL (in-memory)", N, [&] {
        for (std::size_t i = 0; i < N; ++i) {
            memDb.del("key_" + std::to_string(i));
        }
    });

    const std::string benchFile = "bench_snapshot.db";
    if (std::filesystem::exists(benchFile)) std::filesystem::remove(benchFile);

    KVStore diskDb(1000, benchFile, false);
    for (int i = 0; i < 1000; ++i) {
        diskDb.set("k" + std::to_string(i), "v" + std::to_string(i));
    }
    measure("Snapshot Flush (1K)", 1000, [&] { diskDb.flush(); });
    if (std::filesystem::exists(benchFile)) std::filesystem::remove(benchFile);

    constexpr std::size_t keyPoolSize = 10000;
    constexpr std::size_t numThreads = 8;
    constexpr std::size_t opsPerThread = 25000;
    constexpr std::size_t totalConcurrentOps = numThreads * opsPerThread;

    std::vector<std::string> keys;
    std::vector<std::string> vals;
    keys.reserve(keyPoolSize);
    vals.reserve(keyPoolSize);
    for (std::size_t i = 0; i < keyPoolSize; ++i) {
        keys.push_back("hot_key_" + std::to_string(i));
        vals.push_back("hot_val_" + std::to_string(i));
    }

    KVStore coarseDb(20000, "", false);
    ShardedKVStore shardedDb(20000, 16);
    for (std::size_t i = 0; i < keyPoolSize; ++i) {
        coarseDb.set(keys[i], vals[i]);
        shardedDb.set(keys[i], vals[i]);
    }

    std::vector<std::size_t> threadHits(numThreads, 0);

    measure("8T Mixed (1-Mutex)", totalConcurrentOps, [&] {
        std::vector<std::thread> threads;
        threads.reserve(numThreads);
        for (std::size_t t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t] {
                std::size_t localHits = 0;
                for (std::size_t i = 0; i < opsPerThread; ++i) {
                    std::size_t idx = (t * 997 + i * 31) % keyPoolSize;
                    if (i % 5 == 0) {
                        coarseDb.set(keys[idx], vals[idx]);
                    } else if (coarseDb.get(keys[idx]).has_value()) {
                        localHits++;
                    }
                }
                threadHits[t] = localHits;
            });
        }
        for (auto& th : threads) th.join();
    });

    measure("8T Mixed (16-Shards)", totalConcurrentOps, [&] {
        std::vector<std::thread> threads;
        threads.reserve(numThreads);
        for (std::size_t t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t] {
                std::size_t localHits = 0;
                for (std::size_t i = 0; i < opsPerThread; ++i) {
                    std::size_t idx = (t * 997 + i * 31) % keyPoolSize;
                    if (i % 5 == 0) {
                        shardedDb.set(keys[idx], vals[idx]);
                    } else if (shardedDb.get(keys[idx]).has_value()) {
                        localHits++;
                    }
                }
                threadHits[t] = localHits;
            });
        }
        for (auto& th : threads) th.join();
    });

    if (hits != N || threadHits[0] == 0) return 1;
    return 0;
}
