#ifndef SHARDED_KVSTORE_HPP
#define SHARDED_KVSTORE_HPP

#include "KVStore.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ShardedKVStore {
private:
    // Align each shard to a 64-byte CPU cache line to prevent false sharing
    struct alignas(64) Shard {
        KVStore store;
        explicit Shard(std::size_t cap) : store(cap, "", false) {}
    };

    std::vector<std::unique_ptr<Shard>> shards;
    std::size_t totalCapacity = 0;

    std::size_t getShardIndex(const std::string& key) const {
        return std::hash<std::string>{}(key) % shards.size();
    }

public:
    explicit ShardedKVStore(std::size_t capacity, std::size_t numShards = 16) {
        std::size_t count = (numShards == 0 ? 1 : numShards);
        std::size_t cap = (capacity == 0 ? 1 : capacity);
        shards.reserve(count);

        std::size_t baseCap = cap / count;
        std::size_t remainder = cap % count;

        for (std::size_t i = 0; i < count; ++i) {
            std::size_t shardCap = std::max<std::size_t>(1, baseCap + (i < remainder ? 1 : 0));
            shards.push_back(std::make_unique<Shard>(shardCap));
            totalCapacity += shardCap;
        }
    }

    bool set(const std::string& key, const std::string& value, bool verbose = false) {
        return shards[getShardIndex(key)]->store.set(key, value, verbose);
    }

    std::optional<std::string> get(const std::string& key, bool verbose = false) {
        return shards[getShardIndex(key)]->store.get(key, verbose);
    }

    bool del(const std::string& key, bool verbose = false) {
        return shards[getShardIndex(key)]->store.del(key, verbose);
    }

    std::size_t size() const {
        std::size_t total = 0;
        for (const auto& shard : shards) {
            total += shard->store.size();
        }
        return total;
    }

    int hits() const {
        int total = 0;
        for (const auto& shard : shards) {
            total += shard->store.hits();
        }
        return total;
    }

    int misses() const {
        int total = 0;
        for (const auto& shard : shards) {
            total += shard->store.misses();
        }
        return total;
    }

    std::size_t shardCount() const {
        return shards.size();
    }

    void stats() const {
        int totalHits = hits();
        int totalMisses = misses();
        int totalOps = totalHits + totalMisses;
        double hitRate = (totalOps == 0) ? 0.0 : (static_cast<double>(totalHits) / totalOps) * 100.0;

        std::cout << "=== Sharded Engine Statistics ===\n";
        std::cout << "Shards       : " << shards.size() << "\n";
        std::cout << "Active Keys  : " << size() << " / " << totalCapacity << "\n";
        std::cout << "Cache Hits   : " << totalHits << "\n";
        std::cout << "Cache Misses : " << totalMisses << "\n";
        std::cout << "Hit Ratio    : " << std::fixed << std::setprecision(1) << hitRate << "%\n";
    }
};

#endif
