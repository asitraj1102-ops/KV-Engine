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
    size_t hits_ = 0;
    size_t misses_ = 0;
    size_t evictions_ = 0;

    using KeyValuePair = std::pair<std::string, std::string>;
    std::list<KeyValuePair> lru_list_;

    using ListIterator = std::list<KeyValuePair>::iterator;
    std::unordered_map<std::string, ListIterator> cache_map_;

public:
    explicit KVEngine(size_t capacity, const std::string& db_filename = "kv_data.db");
    ~KVEngine();

    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);

    void listAll() const;
    void printStats() const;
    void clear();

    bool saveToDisk() const;
    bool loadFromDisk();
};

#endif
