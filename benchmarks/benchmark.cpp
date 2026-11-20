#include "KVStore.hpp"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

using Clock = std::chrono::steady_clock;

int main() {
    const int N = 50000;
    KVStore memDb(N, "", false);

    auto measure = [](const std::string& label, int count, auto fn) {
        auto t0 = Clock::now();
        fn();
        auto t1 = Clock::now();
        double sec = std::chrono::duration<double>(t1 - t0).count();
        double ops = count / sec;
        double avgUs = (sec * 1e6) / count;
        std::cout << "| " << std::left << std::setw(20) << label
                  << " | " << std::setw(8) << count
                  << " | " << std::fixed << std::setprecision(4) << sec << " s"
                  << " | " << std::setprecision(0) << ops << " ops/s"
                  << " | " << std::setprecision(2) << avgUs << " us |\n";
    };

    std::cout << "| Operation            | Count    | Total Time | Throughput       | Avg Latency |\n";
    std::cout << "| :---                 | :---:    | :---:      | :---:            | :---:       |\n";

    measure("SET (in-memory)", N, [&] {
        for (int i = 0; i < N; ++i) {
            memDb.set("key_" + std::to_string(i), "val_" + std::to_string(i));
        }
    });

    int hits = 0;
    measure("GET (in-memory hit)", N, [&] {
        for (int i = 0; i < N; ++i) {
            if (memDb.get("key_" + std::to_string(i)).has_value()) hits++;
        }
    });

    measure("DEL (in-memory)", N, [&] {
        for (int i = 0; i < N; ++i) {
            memDb.del("key_" + std::to_string(i));
        }
    });

    // Disk persistence snapshot benchmark (1,000 keys flushed to disk)
    const std::string benchFile = "bench_snapshot.db";
    if (std::filesystem::exists(benchFile)) std::filesystem::remove(benchFile);
    KVStore diskDb(1000, benchFile, false);
    for (int i = 0; i < 1000; ++i) {
        diskDb.set("k" + std::to_string(i), "v" + std::to_string(i));
    }
    measure("Snapshot Flush (1K)", 1000, [&] { diskDb.flush(); });
    if (std::filesystem::exists(benchFile)) std::filesystem::remove(benchFile);

    if (hits != N) return 1;
    return 0;
}