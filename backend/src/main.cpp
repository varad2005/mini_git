#include "../include/Repository.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: mygit <command> [args]\n" << std::endl;
        std::cout << "Mini Git - Simple Snapshot System\n" << std::endl;
        std::cout << "COMMANDS:" << std::endl;
        std::cout << "  init              Start tracking this folder for snapshots" << std::endl;
        std::cout << "  status            Show what has changed in your folder since the last save" << std::endl;
        std::cout << "  add <file|.>      Prepare a file (or all files) to be saved in a snapshot" << std::endl;
        std::cout << "  commit <msg>      Permanently save your prepared files as a new project version" << std::endl;
        std::cout << "  log               View the timeline of all saved snapshots" << std::endl;
        std::cout << "  branch <name>     Create a new project path to experiment in" << std::endl;
        std::cout << "  checkout <id>     Jump back to a specific version or branch" << std::endl;
        std::cout << "  diff <c1> <c2>    Compare two different versions line-by-line" << std::endl;
        std::cout << "  tag <name> [id]   Give a permanent name (like v1.0) to a snapshot" << std::endl;
        std::cout << "  stash <push|pop>  Temporarily save or restore changes without committing" << std::endl;
        std::cout << "  reset <id>        Move current branch back to an old snapshot" << std::endl;
        std::cout << "  graph             Export history for the web interface" << std::endl;
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
    } else if (command == "status") {
        Repository::status();
    } else if (command == "tag") {
        if (argc < 3) {
            std::cerr << "Usage: mygit tag <name> [commitId]" << std::endl;
            return 1;
        }
        Repository::tag(argv[2], argc > 3 ? argv[3] : "");
    } else if (command == "stash") {
        if (argc < 3) {
            std::cerr << "Usage: mygit stash <push|pop>" << std::endl;
            return 1;
        }
        std::string sub = argv[2];
        if (sub == "push") Repository::stashPush();
        else if (sub == "pop") Repository::stashPop();
        else std::cerr << "Unknown stash command: " << sub << std::endl;
    } else if (command == "reset") {
        if (argc < 3) {
            std::cerr << "Usage: mygit reset <commitId> [--hard]" << std::endl;
            return 1;
        }
        bool hard = true;
        if (argc > 3 && std::string(argv[3]) == "--soft") hard = false;
        Repository::reset(argv[2], hard);
    } else if (command == "merge") {
        if (argc < 3) {
            std::cerr << "Usage: mygit merge <branch|commitId>" << std::endl;
            return 1;
        }
        Repository::merge(argv[2]);
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
