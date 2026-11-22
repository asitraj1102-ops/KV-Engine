#ifndef KVSTORE_HPP
#define KVSTORE_HPP

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

class KVStore {
private:
    std::size_t capacity;
    std::string dbFile;
    bool autoPersist;
    int cacheHits = 0;
    int cacheMisses = 0;
    mutable std::mutex mtx;

    // Front = Most Recently Used (MRU), Back = Least Recently Used (LRU)
    std::list<std::pair<std::string, std::string>> lruList;
    std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>::iterator> cacheMap;

    // Private helper: caller must already hold `mtx` (prevents self-deadlock)
    void saveToDisk() const {
        if (dbFile.empty()) return;
        std::ofstream out(dbFile, std::ios::trunc);
        if (!out) return;
        for (const auto& item : lruList) {
            out << item.first << " " << item.second << "\n";
        }
    }

    void loadFromDisk() {
        if (dbFile.empty()) return;
        std::ifstream in(dbFile);
        if (!in) return;
        std::string line;
        while (std::getline(in, line)) {
            auto pos = line.find(' ');
            if (pos == std::string::npos || pos == 0) continue;
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            if (value.empty()) continue;
            if (cacheMap.find(key) != cacheMap.end()) continue;
            if (lruList.size() >= capacity) break;

            // push_back preserves exact MRU -> LRU order saved on disk
            lruList.push_back({key, value});
            cacheMap[key] = std::prev(lruList.end());
        }
    }

public:
    explicit KVStore(std::size_t cap, const std::string& filename = "data.txt", bool persist = true)
        : capacity(cap == 0 ? 1 : cap), dbFile(filename), autoPersist(persist) {
        if (autoPersist) {
            loadFromDisk();
        }
    }

    bool set(const std::string& key, const std::string& value, bool verbose = false) {
        if (key.empty() || value.empty()) return false;
        std::lock_guard<std::mutex> lock(mtx);

        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            it->second->second = value;
            lruList.splice(lruList.begin(), lruList, it->second);
            if (autoPersist) saveToDisk();
            if (verbose) std::cout << "[UPDATED] " << key << " = " << value << "\n";
            return true;
        }

        if (lruList.size() >= capacity) {
            std::string oldKey = lruList.back().first;
            lruList.pop_back();
            cacheMap.erase(oldKey);
            if (verbose) {
                std::cout << "[LRU EVICTED] Cache full! Removed least recently used key: '" << oldKey << "'\n";
            }
        }

        lruList.push_front({key, value});
        cacheMap[key] = lruList.begin();

        if (autoPersist) saveToDisk();
        if (verbose) std::cout << "[OK] Saved " << key << " = " << value << "\n";
        return true;
    }

    std::optional<std::string> get(const std::string& key, bool verbose = false) {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) {
            cacheMisses++;
            if (verbose) std::cout << "(nil) Key '" << key << "' not found.\n";
            return std::nullopt;
        }

        cacheHits++;
        lruList.splice(lruList.begin(), lruList, it->second);
        if (verbose) std::cout << "\"" << it->second->second << "\"\n";
        return it->second->second;
    }

    bool del(const std::string& key, bool verbose = false) {
        std::lock_guard<std::mutex> lock(mtx);

        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) {
            if (verbose) std::cout << "[ERROR] Key '" << key << "' does not exist.\n";
            return false;
        }

        lruList.erase(it->second);
        cacheMap.erase(it);
        if (autoPersist) saveToDisk();
        if (verbose) std::cout << "[DELETED] Removed '" << key << "'.\n";
        return true;
    }

    void flush() const {
        std::lock_guard<std::mutex> lock(mtx);
        saveToDisk();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return lruList.size();
    }

    int hits() const {
        std::lock_guard<std::mutex> lock(mtx);
        return cacheHits;
    }

    int misses() const {
        std::lock_guard<std::mutex> lock(mtx);
        return cacheMisses;
    }

    void listAll() const {
        std::lock_guard<std::mutex> lock(mtx);

        if (lruList.empty()) {
            std::cout << "(empty database)\n";
            return;
        }
        std::cout << "--- Cache Order (Most Recent to Least Recent) ---\n";
        int idx = 1;
        for (const auto& item : lruList) {
            std::cout << idx++ << ") " << item.first << " : " << item.second << "\n";
        }
    }

    void stats() const {
        std::lock_guard<std::mutex> lock(mtx);

        int total = cacheHits + cacheMisses;
        double hitRate = (total == 0) ? 0.0 : (static_cast<double>(cacheHits) / total) * 100.0;

        std::cout << "=== Engine Statistics ===\n";
        std::cout << "Active Keys  : " << lruList.size() << " / " << capacity << "\n";
        std::cout << "Cache Hits   : " << cacheHits << "\n";
        std::cout << "Cache Misses : " << cacheMisses << "\n";
        std::cout << "Hit Ratio    : " << std::fixed << std::setprecision(1) << hitRate << "%\n";
        std::cout << "Disk File    : " << dbFile << "\n";
    }
};

#endif
