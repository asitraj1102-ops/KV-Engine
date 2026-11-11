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

bool KVEngine::isExpired(const Entry& entry) const {
    if (!entry.expiry.has_value()) return false;
    return Clock::now() >= entry.expiry.value();
}

void KVEngine::purgeExpired() {
    for (auto it = lru_list_.begin(); it != lru_list_.end(); ) {
        if (isExpired(*it)) {
            cache_map_.erase(it->key);
            it = lru_list_.erase(it);
        } else {
            ++it;
        }
    }
}

bool KVEngine::set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds, bool silent) {
    auto it = cache_map_.find(key);
    std::optional<TimePoint> expiry_time = std::nullopt;
    if (ttl_seconds.has_value() && ttl_seconds.value() > 0) {
        expiry_time = Clock::now() + std::chrono::seconds(ttl_seconds.value());
    }

    if (it != cache_map_.end()) {
        it->second->value = value;
        it->second->expiry = expiry_time;
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
        return false;
    }

    purgeExpired();

    if (lru_list_.size() >= capacity_) {
        std::string lru_key = lru_list_.back().key;
        if (!silent) {
            std::cout << "[LRU Eviction] Capacity full! Removed least recently used key: '"
                      << lru_key << "'\n";
        }
        cache_map_.erase(lru_key);
        lru_list_.pop_back();
        evictions_++;
    }

    lru_list_.push_front({key, value, expiry_time});
    cache_map_[key] = lru_list_.begin();
    return true;
}

std::optional<std::string> KVEngine::get(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
        misses_++;
        return std::nullopt;
    }

    if (isExpired(*(it->second))) {
        std::cout << "[TTL Expired] Key '" << key << "' has expired and was auto-deleted.\n";
        lru_list_.erase(it->second);
        cache_map_.erase(it);
        misses_++;
        return std::nullopt;
    }

    hits_++;
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
    return it->second->value;
}

bool KVEngine::del(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) return false;
    lru_list_.erase(it->second);
    cache_map_.erase(it);
    return true;
}

bool KVEngine::expire(const std::string& key, int seconds) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end() || isExpired(*(it->second))) return false;
    it->second->expiry = Clock::now() + std::chrono::seconds(seconds);
    return true;
}

long long KVEngine::ttl(const std::string& key) {
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) return -2; // Key does not exist
    if (isExpired(*(it->second))) {
        lru_list_.erase(it->second);
        cache_map_.erase(it);
        return -2;
    }
    if (!it->second->expiry.has_value()) return -1; // No expiry set
    return std::chrono::duration_cast<std::chrono::seconds>(
               it->second->expiry.value() - Clock::now()).count();
}

void KVEngine::listAll() {
    purgeExpired();
    if (lru_list_.empty()) {
        std::cout << "(empty database)\n";
        return;
    }
    std::cout << "\n--- Current Cache Order (MRU to LRU) ---\n";
    int index = 1;
    for (const auto& entry : lru_list_) {
        std::cout << index++ << ". [" << entry.key << "] = \"" << entry.value << "\"";
        if (entry.expiry.has_value()) {
            auto rem = std::chrono::duration_cast<std::chrono::seconds>(
                           entry.expiry.value() - Clock::now()).count();
            std::cout << " (expires in " << (rem > 0 ? rem : 0) << "s)";
        }
        std::cout << "\n";
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

void KVEngine::runBenchmark(size_t num_ops) {
    std::cout << "\n[Benchmark] Running " << num_ops << " O(1) SET + GET operations in RAM...\n";
    KVEngine temp_engine(10000, "bench_temp.db");

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < num_ops; ++i) {
        std::string k = "key_" + std::to_string(i % 10000);
        temp_engine.set(k, "val", std::nullopt, true);
        temp_engine.get(k);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::remove("bench_temp.db");

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double ops_per_sec = (num_ops * 2.0) / (ms / 1000.0);

    std::cout << "----------------------------------------------------\n";
    std::cout << "Total Operations : " << (num_ops * 2) << " (" << num_ops << " SETs + " << num_ops << " GETs)\n";
    std::cout << "Time Taken       : " << std::fixed << std::setprecision(2) << ms << " ms\n";
    std::cout << "Throughput       : ~" << static_cast<size_t>(ops_per_sec) << " operations/sec\n";
    std::cout << "----------------------------------------------------\n\n";
}

void KVEngine::clear() {
    lru_list_.clear();
    cache_map_.clear();
    std::cout << "OK (In-memory cache cleared)\n";
}

bool KVEngine::saveToDisk() const {
    std::ofstream out(db_filename_, std::ios::trunc);
    if (!out.is_open()) return false;
    for (auto it = lru_list_.rbegin(); it != lru_list_.rend(); ++it) {
        if (!isExpired(*it)) {
            out << it->key << "\t" << it->value << "\n";
        }
    }
    return true;
}

bool KVEngine::loadFromDisk() {
    std::ifstream in(db_filename_);
    if (!in.is_open()) return false;
    std::string line;
    size_t loaded_count = 0;
    while (std::getline(in, line)) {
        size_t tab_pos = line.find('\t');
        if (tab_pos != std::string::npos) {
            std::string key = line.substr(0, tab_pos);
            std::string value = line.substr(tab_pos + 1);
            set(key, value, std::nullopt, true);
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
