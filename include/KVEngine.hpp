#ifndef KV_ENGINE_HPP
#define KV_ENGINE_HPP

#include <string>
#include <list>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <cstddef>

class KVEngine {
private:
    size_t capacity_;
    std::string db_filename_;
    size_t hits_ = 0;
    size_t misses_ = 0;
    size_t evictions_ = 0;

    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    struct Entry {
        std::string key;
        std::string value;
        std::optional<TimePoint> expiry;
    };

    std::list<Entry> lru_list_;
    using ListIterator = std::list<Entry>::iterator;
    std::unordered_map<std::string, ListIterator> cache_map_;

    bool isExpired(const Entry& entry) const;
    void purgeExpired();

public:
    explicit KVEngine(size_t capacity, const std::string& db_filename = "kv_data.db");
    ~KVEngine();

    bool set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds = std::nullopt, bool silent = false);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool expire(const std::string& key, int seconds);
    long long ttl(const std::string& key);

    void listAll();
    void printStats() const;
    void runBenchmark(size_t num_ops = 100000);
    void clear();

    bool saveToDisk() const;
    bool loadFromDisk();
};

#endif
