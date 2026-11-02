#include "KVEngine.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

KVEngine::KVEngine(size_t capacity, const std::string& db_filename)
    : capacity_(capacity > 0 ? capacity : 5), db_filename_(db_filename) {
    loadFromDisk();
}

KVEngine::~KVEngine() {
    saveToDisk();
}

bool KVEngine::set(const std::string& key, const std::string& value) {
    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
        it->second->second = value;
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
        return false;
    }

    if (lru_list_.size() >= capacity_) {
        std::string lru_key = lru_list_.back().first;
        std::cout << "[LRU Eviction] Capacity full! Removed least recently used key: '"
                  << lru_key << "'\n";
        cache_map_.erase(lru_key);
        lru_list_.pop_back();
        evictions_++;
    }

    lru_list_.emplace_front(key, value);
    cache_map_[key] = lru_list_.begin();
    return true;
}

std::optional<std::string> KVEngine::get(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        misses_++;
        return std::nullopt;
    }
    hits_++;
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
    return it->second->second;
}

bool KVEngine::del(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        return false;
    }
    lru_list_.erase(it->second);
    cache_map_.erase(it);
    return true;
}

void KVEngine::listAll() const {
    if (lru_list_.empty()) {
        std::cout << "(empty database)\n";
        return;
    }
    std::cout << "\n--- Current Cache Order (MRU to LRU) ---\n";
    int index = 1;
    for (const auto& pair : lru_list_) {
        std::cout << index++ << ". [" << pair.first << "] = \"" << pair.second << "\"\n";
    }
    std::cout << "----------------------------------------\n";
}

void KVEngine::printStats() const {
    size_t total_requests = hits_ + misses_;
    double hit_rate = (total_requests > 0)
                          ? (static_cast<double>(hits_) / total_requests) * 100.0
                          : 0.0;

    std::cout << "\n========= KV-Engine Statistics =========\n";
    std::cout << "Storage Usage : " << lru_list_.size() << " / " << capacity_ << " keys\n";
    std::cout << "Cache Hits    : " << hits_ << "\n";
    std::cout << "Cache Misses  : " << misses_ << "\n";
    std::cout << "LRU Evictions : " << evictions_ << "\n";
    std::cout << "Hit Ratio     : " << std::fixed << std::setprecision(2) << hit_rate << "%\n";
    std::cout << "========================================\n";
}

void KVEngine::clear() {
    lru_list_.clear();
    cache_map_.clear();
    std::cout << "OK (In-memory cache cleared)\n";
}

bool KVEngine::saveToDisk() const {
    std::ofstream out(db_filename_, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    for (auto it = lru_list_.rbegin(); it != lru_list_.rend(); ++it) {
        out << it->first << "\t" << it->second << "\n";
    }
    return true;
}

bool KVEngine::loadFromDisk() {
    std::ifstream in(db_filename_);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    size_t loaded_count = 0;
    while (std::getline(in, line)) {
        size_t tab_pos = line.find('\t');
        if (tab_pos != std::string::npos) {
            std::string key = line.substr(0, tab_pos);
            std::string value = line.substr(tab_pos + 1);
            set(key, value);
            loaded_count++;
        }
    }
    evictions_ = 0;
    if (loaded_count > 0) {
        std::cout << "[Disk] Loaded " << lru_list_.size() << " record(s) from '"
                  << db_filename_ << "'.\n";
    }
    return true;
}
