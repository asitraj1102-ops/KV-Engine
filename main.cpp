#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include "KVStore.hpp"

void printHelp() {
    std::cout << "\nAvailable Commands:\n"
              << "  SET <key> <value>   - Store or update a key-value pair\n"
              << "  GET <key>           - Retrieve value by key (promotes to MRU)\n"
              << "  DEL <key>           - Delete a key from memory and disk\n"
              << "  LIST                - Show all keys from Most to Least Recently Used\n"
              << "  STATS               - View cache capacity and Hit/Miss ratio\n"
              << "  HELP                - Show this menu\n"
              << "  EXIT                - Save and quit\n";
}

int main() {
    KVStore db(5, "data.txt", true);

    std::cout << "==================================================\n";
    std::cout << "   KV-Engine v1.0 (In-Memory LRU Store in C++17)  \n";
    std::cout << "==================================================\n";
    std::cout << "Type 'HELP' for commands or 'EXIT' to quit.\n";

    std::string line;
    while (true) {
        std::cout << "\nkv-engine> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;
        if (cmd.empty()) continue;

        for (char& c : cmd) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        if (cmd == "SET") {
            std::string key, value;
            ss >> key;
            std::getline(ss >> std::ws, value);
            if (key.empty() || value.empty()) {
                std::cout << "Usage: SET <key> <value>\n";
            } else {
                db.set(key, value, true);
            }
        } else if (cmd == "GET") {
            std::string key;
            ss >> key;
            if (key.empty()) {
                std::cout << "Usage: GET <key>\n";
            } else {
                db.get(key, true);
            }
        } else if (cmd == "DEL") {
            std::string key;
            ss >> key;
            if (key.empty()) {
                std::cout << "Usage: DEL <key>\n";
            } else {
                db.del(key, true);
            }
        } else if (cmd == "LIST") {
            db.listAll();
        } else if (cmd == "STATS") {
            db.stats();
        } else if (cmd == "HELP") {
            printHelp();
        } else if (cmd == "EXIT") {
            db.flush();
            std::cout << "Shutting down KV-Engine. All data persisted to disk.\n";
            break;
        } else {
            std::cout << "Unknown command '" << cmd << "'. Type HELP for available commands.\n";
        }
    }

    return 0;
}