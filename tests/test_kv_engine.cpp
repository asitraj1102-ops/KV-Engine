#include "KVStore.hpp"
#include "ShardedKVStore.hpp"
#include <thread>
#include <vector>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static void cleanup(const std::string& path) {
    if (fs::exists(path)) fs::remove(path);
}

int main() {
    const std::string dbFile = "test_kvstore.db";

    // 1. Basic SET & GET
    cleanup(dbFile);
    {
        KVStore db(5, dbFile);
        assert(db.set("name", "asit"));
        assert(db.get("name").value_or("") == "asit");
    }

    // 2. Missing key returns nullopt & increments misses
    {
        KVStore db(5, dbFile);
        assert(!db.get("missing").has_value());
        assert(db.misses() == 1);
    }

    // 3. Overwrite existing key
    {
        KVStore db(5, dbFile);
        assert(db.set("name", "asit raj"));
        assert(db.get("name").value_or("") == "asit raj");
    }

    // 4. Delete existing key
    {
        KVStore db(5, dbFile);
        assert(db.del("name"));
        assert(!db.get("name").has_value());
    }

    // 5. Delete missing key returns false safely
    {
        KVStore db(5, dbFile);
        assert(!db.del("ghost"));
    }

    // 6. Empty key or value rejected cleanly
    {
        KVStore db(5, dbFile);
        assert(!db.set("", "val"));
        assert(!db.set("key", ""));
    }

    // 7. LRU eviction order (oldest unused removed first)
    cleanup(dbFile);
    {
        KVStore db(2, dbFile);
        db.set("k1", "v1");
        db.set("k2", "v2");
        db.set("k3", "v3"); // evicts k1
        assert(!db.get("k1").has_value());
        assert(db.get("k2").value_or("") == "v2");
        assert(db.get("k3").value_or("") == "v3");
    }

    // 8. GET promotes key to MRU
    cleanup(dbFile);
    {
        KVStore db(2, dbFile);
        db.set("k1", "v1");
        db.set("k2", "v2");
        assert(db.get("k1").value_or("") == "v1"); // k1 becomes MRU
        db.set("k3", "v3");                        // evicts k2
        assert(db.get("k1").value_or("") == "v1");
        assert(!db.get("k2").has_value());
    }

    // 9. Overwrite promotes key to MRU
    cleanup(dbFile);
    {
        KVStore db(2, dbFile);
        db.set("k1", "v1");
        db.set("k2", "v2");
        db.set("k1", "v1_updated"); // k1 becomes MRU
        db.set("k3", "v3");         // evicts k2
        assert(db.get("k1").value_or("") == "v1_updated");
        assert(!db.get("k2").has_value());
    }

    // 10. Capacity = 0 safely clamped to 1
    cleanup(dbFile);
    {
        KVStore db(0, dbFile);
        db.set("a", "1");
        db.set("b", "2");
        assert(db.size() == 1);
        assert(!db.get("a").has_value());
        assert(db.get("b").value_or("") == "2");
    }

    // 11. Persistence survives restart (including spaces in values)
    cleanup(dbFile);
    {
        KVStore db(5, dbFile);
        db.set("lang", "C++17 Systems");
        db.set("env", "WSL2 Ubuntu");
    }
    {
        KVStore db(5, dbFile);
        assert(db.get("lang").value_or("") == "C++17 Systems");
        assert(db.get("env").value_or("") == "WSL2 Ubuntu");
    }

    // 12. Reload preserves exact MRU -> LRU order (regression test!)
    cleanup(dbFile);
    {
        KVStore db(2, dbFile);
        db.set("old_key", "1");
        db.set("new_key", "2"); // new_key = MRU, old_key = LRU
    }
    {
        KVStore db(2, dbFile);
        db.set("third_key", "3"); // must evict old_key, NOT new_key
        assert(!db.get("old_key").has_value());
        assert(db.get("new_key").value_or("") == "2");
        assert(db.get("third_key").value_or("") == "3");
    }

    // 13. Deleted key remains deleted after restart
    {
        KVStore db(5, dbFile);
        assert(db.del("new_key"));
    }
    {
        KVStore db(5, dbFile);
        assert(!db.get("new_key").has_value());
        assert(db.get("third_key").value_or("") == "3");
    }

    // 14. Hit/Miss stats tracking
    cleanup(dbFile);
    {
        KVStore db(3, dbFile, false);
        db.set("x", "10");
        assert(db.get("x").value_or("") == "10");
        assert(!db.get("y").has_value());
        assert(db.hits() == 1);
        assert(db.misses() == 1);
    }

    // 15. In-memory stress test (10,000 sequential ops)
    {
        KVStore db(10000, "", false);
        for (int i = 0; i < 10000; ++i) {
            db.set("k" + std::to_string(i), "v" + std::to_string(i));
        }
        for (int i = 0; i < 10000; ++i) {
            assert(db.get("k" + std::to_string(i)).value_or("") == "v" + std::to_string(i));
        }
        assert(db.size() == 10000);
    }

    // 16. ShardedKVStore basic operations and stats
    {
        ShardedKVStore sdb(64, 8);
        assert(sdb.shardCount() == 8);
        assert(sdb.set("alpha", "100"));
        assert(sdb.set("beta", "200"));
        assert(sdb.get("alpha").value_or("") == "100");
        assert(!sdb.get("missing").has_value());
        assert(sdb.hits() == 1);
        assert(sdb.misses() == 1);
        assert(sdb.del("alpha"));
        assert(!sdb.del("alpha"));
        assert(sdb.size() == 1);
    }

    // 17. Concurrent KVStore stress test (8 threads, overlapping keys + LRU eviction)
    {
        KVStore db(64, "", false);
        constexpr int numThreads = 8;
        constexpr int opsPerThread = 1000;
        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&db, t]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    std::string key = "shared_k" + std::to_string((t * 31 + i) % 128);
                    db.set(key, "val_" + std::to_string(t) + "_" + std::to_string(i));
                    (void)db.get(key);
                    if (i % 5 == 0) {
                        db.del(key);
                    }
                }
            });
        }
        for (auto& th : threads) th.join();

        assert(db.size() <= 64);
        assert(db.hits() + db.misses() == numThreads * opsPerThread);
    }

    // 18. Concurrent ShardedKVStore stress test (disjoint integrity + heavy contention)
    {
        ShardedKVStore sdb(32000, 16);
        constexpr int numThreads = 8;
        constexpr int keysPerThread = 500;
        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&sdb, t]() {
                for (int i = 0; i < keysPerThread; ++i) {
                    std::string key = "t" + std::to_string(t) + "_k" + std::to_string(i);
                    std::string val = "v" + std::to_string(t) + "_" + std::to_string(i);
                    assert(sdb.set(key, val));
                }
                for (int i = 0; i < keysPerThread; ++i) {
                    std::string key = "t" + std::to_string(t) + "_k" + std::to_string(i);
                    std::string val = "v" + std::to_string(t) + "_" + std::to_string(i);
                    assert(sdb.get(key).value_or("") == val);
                }
            });
        }
        for (auto& th : threads) th.join();

        assert(sdb.size() == numThreads * keysPerThread);
        assert(sdb.hits() == numThreads * keysPerThread);
        assert(sdb.misses() == 0);

        ShardedKVStore hotCache(64, 8);
        constexpr int opsPerThread = 1000;
        threads.clear();

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&hotCache, t]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    std::string key = "hot_" + std::to_string((t * 17 + i) % 100);
                    hotCache.set(key, "v" + std::to_string(i));
                    (void)hotCache.get(key);
                    if (i % 4 == 0) {
                        hotCache.del(key);
                    }
                }
            });
        }
        for (auto& th : threads) th.join();

        assert(hotCache.size() <= 64);
        assert(hotCache.hits() + hotCache.misses() == numThreads * opsPerThread);
    }

    cleanup(dbFile);
    std::cout << "ALL 18 KVSTORE UNIT TESTS PASSED!\n";
    return 0;
}