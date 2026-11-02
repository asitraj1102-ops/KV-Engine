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
              << "  SET <key> <value>  - Insert or update a key-value pair\n"
              << "  GET <key>          - Retrieve value of a key in O(1) time\n"
              << "  DEL <key>          - Delete a key from the store\n"
              << "  LIST               - Show all keys from Most to Least Recently Used\n"
              << "  STATS              - Display cache hits, misses, hit ratio & evictions\n"
              << "  SAVE               - Manually flush current data to disk\n"
              << "  CLEAR              - Remove all keys from memory\n"
              << "  HELP               - Show this command menu\n"
              << "  EXIT               - Save data to disk and quit\n\n";
}

int main(int argc, char* argv[]) {
    size_t capacity = 5;
    if (argc > 1) {
        try {
            capacity = std::stoul(argv[1]);
        } catch (...) {
            std::cerr << "Invalid capacity argument. Using default capacity: 5\n";
        }
    }

    std::cout << "====================================================\n";
    std::cout << "   KV-Engine (Mini-Redis CLI) | C++17 LRU Storage   \n";
    std::cout << "   Max Cache Capacity: " << capacity << " keys (Type HELP for info)\n";
    std::cout << "====================================================\n";

    KVEngine engine(capacity);
    std::string line;

    while (true) {
        std::cout << "kv-engine> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::istringstream iss(line);
        std::string cmd;
        if (!(iss >> cmd)) {
            continue;
        }

        cmd = toUpper(cmd);

        if (cmd == "SET") {
            std::string key, value;
            if (!(iss >> key)) {
                std::cout << "(error) Usage: SET <key> <value>\n";
                continue;
            }
            std::getline(iss >> std::ws, value);
            if (value.empty()) {
                std::cout << "(error) Value cannot be empty. Usage: SET <key> <value>\n";
                continue;
            }
            bool is_new = engine.set(key, value);
            std::cout << (is_new ? "OK (inserted)\n" : "OK (updated)\n");

        } else if (cmd == "GET") {
            std::string key;
            if (!(iss >> key)) {
                std::cout << "(error) Usage: GET <key>\n";
                continue;
            }
            auto val = engine.get(key);
            if (val.has_value()) {
                std::cout << "\"" << val.value() << "\"\n";
            } else {
                std::cout << "(nil)\n";
            }

        } else if (cmd == "DEL") {
            std::string key;
            if (!(iss >> key)) {
                std::cout << "(error) Usage: DEL <key>\n";
                continue;
            }
            if (engine.del(key)) {
                std::cout << "(integer) 1\n";
            } else {
                std::cout << "(integer) 0 (key not found)\n";
            }

        } else if (cmd == "LIST") {
            engine.listAll();

        } else if (cmd == "STATS") {
            engine.printStats();

        } else if (cmd == "SAVE") {
            if (engine.saveToDisk()) {
                std::cout << "OK (Database persisted to 'kv_data.db')\n";
            } else {
                std::cout << "(error) Failed to write to disk.\n";
            }

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
