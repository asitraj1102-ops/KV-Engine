#ifndef KV_ENGINE_HPP
#define KV_ENGINE_HPP

#include <string>
#include <list>
#include <unordered_map>
#include <optional>
#include <cstddef>

class KVEngine {
private:
    size_t capacity_;
    std::string db_filename_;

    // Performance Metrics
    size_t hits_ = 0;
    size_t misses_ = 0;
    size_t evictions_ = 0;

    // Doubly Linked List stores {key, value} pairs
    // Front = Most Recently Used (MRU), Back = Least Recently Used (LRU)
    using KeyValuePair = std::pair<std::string, std::string>;
    std::list<KeyValuePair> lru_list_;

    // Hash Map stores Key -> Iterator to the node in lru_list_ for O(1) access
    using ListIterator = std::list<KeyValuePair>::iterator;
    std::unordered_map<std::string, ListIterator> cache_map_;

public:
    explicit KVEngine(size_t capacity, const std::string& db_filename = "kv_data.db");
    ~KVEngine();

    // Core O(1) Operations
    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);

    // Utility & Inspection Operations
    void listAll() const;
    void printStats() const;
    void clear();

    // Disk Persistence Operations
    bool saveToDisk() const;
    bool loadFromDisk();
};

#endif // KV_ENGINE_HPP