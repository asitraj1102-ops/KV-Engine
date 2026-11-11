#include "KVEngine.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string toUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return str;
}

void printHelp() {
    std::cout << "\nAvailable Commands:\n"
              << "  SET <key> <value>     - Insert or update a key-value pair in O(1)\n"
              << "  GET <key>             - Retrieve value of a key in O(1)\n"
              << "  DEL <key>             - Delete a key from the store\n"
              << "  EXPIRE <key> <sec>    - Set automatic expiration timer (TTL) in seconds\n"
              << "  TTL <key>             - Check remaining time-to-live for a key\n"
              << "  LIST                  - Show all active keys (Most to Least Recently Used)\n"
              << "  STATS                 - Display cache hits, misses, hit ratio & evictions\n"
              << "  BENCHMARK             - Run 200,000 live O(1) operations speed test\n"
              << "  SAVE                  - Manually flush current data to disk\n"
              << "  CLEAR                 - Remove all keys from memory\n"
              << "  EXIT                  - Save data to disk and quit\n\n";
}

int main(int argc, char* argv[]) {
    size_t capacity = 5;
    if (argc > 1) {
        try { capacity = std::stoul(argv[1]); } catch (...) {}
    }

    std::cout << "========================================================\n";
    std::cout << "   KV-Engine v2.0 (Mini-Redis CLI) | C++17 Storage      \n";
    std::cout << "   Features: O(1) LRU Cache | TTL Expiry | Disk Engine  \n";
    std::cout << "   Max Cache Capacity: " << capacity << " keys (Type HELP for commands)\n";
    std::cout << "========================================================\n";

    KVEngine engine(capacity);
    std::string line;

    while (true) {
        std::cout << "kv-engine> ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) continue;
        cmd = toUpper(cmd);

        if (cmd == "SET") {
            std::string key, value;
            if (!(iss >> key)) { std::cout << "(error) Usage: SET <key> <value>\n"; continue; }
            std::getline(iss >> std::ws, value);
            if (value.empty()) { std::cout << "(error) Value cannot be empty.\n"; continue; }
            bool is_new = engine.set(key, value);
            std::cout << (is_new ? "OK (inserted)\n" : "OK (updated)\n");

        } else if (cmd == "GET") {
            std::string key;
            if (!(iss >> key)) { std::cout << "(error) Usage: GET <key>\n"; continue; }
            auto val = engine.get(key);
            if (val.has_value()) std::cout << "\"" << val.value() << "\"\n";
            else std::cout << "(nil)\n";

        } else if (cmd == "DEL") {
            std::string key;
            if (!(iss >> key)) { std::cout << "(error) Usage: DEL <key>\n"; continue; }
            std::cout << (engine.del(key) ? "(integer) 1\n" : "(integer) 0 (not found)\n");

        } else if (cmd == "EXPIRE") {
            std::string key; int sec;
            if (!(iss >> key >> sec) || sec <= 0) {
                std::cout << "(error) Usage: EXPIRE <key> <positive_seconds>\n"; continue;
            }
            if (engine.expire(key, sec)) std::cout << "(integer) 1 (expires in " << sec << "s)\n";
            else std::cout << "(integer) 0 (key not found)\n";

        } else if (cmd == "TTL") {
            std::string key;
            if (!(iss >> key)) { std::cout << "(error) Usage: TTL <key>\n"; continue; }
            long long rem = engine.ttl(key);
            if (rem == -2) std::cout << "(integer) -2 (key does not exist or expired)\n";
            else if (rem == -1) std::cout << "(integer) -1 (no expiry set)\n";
            else std::cout << "(integer) " << rem << "s remaining\n";

        } else if (cmd == "LIST") {
            engine.listAll();
        } else if (cmd == "STATS") {
            engine.printStats();
        } else if (cmd == "BENCHMARK") {
            engine.runBenchmark();
        } else if (cmd == "SAVE") {
            std::cout << (engine.saveToDisk() ? "OK (Saved to 'kv_data.db')\n" : "(error) Save failed.\n");
        } else if (cmd == "CLEAR") {
            engine.clear();
        } else if (cmd == "HELP") {
            printHelp();
        } else if (cmd == "EXIT" || cmd == "QUIT") {
            std::cout << "Saving data to disk and shutting down... Goodbye!\n";
            break;
        } else {
            std::cout << "(error) Unknown command '" << cmd << "'. Type HELP.\n";
        }
    }
    return 0;
}
