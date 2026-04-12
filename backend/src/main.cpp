#include "Repository.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: mygit <command> [args]" << std::endl;
        std::cout << "Commands: init, add <file>, commit <message>, log, branch <name>, checkout <name>, diff <c1> <c2>" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        Repository::init();
    } else if (command == "add") {
        if (argc < 3) {
            std::cerr << "Usage: mygit add <filename>" << std::endl;
            return 1;
        }
        Repository::add(argv[2]);
    } else if (command == "commit") {
        if (argc < 3) {
            std::cerr << "Usage: mygit commit <message>" << std::endl;
            return 1;
        }
        Repository::commit(argv[2]);
    } else if (command == "log") {
        Repository::log();
    } else if (command == "branch") {
        if (argc < 3) {
            std::cerr << "Usage: mygit branch <name>" << std::endl;
            return 1;
        }
        Repository::branch(argv[2]);
    } else if (command == "checkout") {
        if (argc < 3) {
            std::cerr << "Usage: mygit checkout <name>" << std::endl;
            return 1;
        }
        Repository::checkout(argv[2]);
    } else if (command == "diff") {
        if (argc < 4) {
             std::cerr << "Usage: mygit diff <commit1> <commit2>" << std::endl;
             return 1;
        }
        Repository::diff(argv[2], argv[3]);
    } else if (command == "graph") {
        std::cout << Repository::getGraph() << std::endl;
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}
