#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <set>

namespace fs = std::filesystem;

class SimpleGit {
public:
    // 1. INIT: Setup the storage
    void initRepo() {
        if (fs::exists(".mygit")) {
            std::cout << "Repository already initialized." << std::endl;
            return;
        }
        try {
            fs::create_directory(".mygit");
            fs::create_directory(".mygit/commits");
            
            updateHead(0);
            std::cout << "Initialized empty SimpleGit repository." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error initializing repo: " << e.what() << std::endl;
        }
    }

    // 2. COMMIT: Full folder snapshot with detailed metadata
    void commitChanges(std::string message) {
        if (!fs::exists(".mygit")) {
            std::cerr << "Error: Not a SimpleGit repository." << std::endl;
            return;
        }

        int nextId = getCurrentHead() + 1;
        std::string commitDir = ".mygit/commits/commit_" + std::to_string(nextId);
        
        try {
            fs::create_directory(commitDir);

            // Copy all non-ignored project files
            for (const auto& entry : fs::directory_iterator(".")) {
                std::string filename = entry.path().filename().string();
                if (!isIgnored(filename)) {
                    fs::copy(entry.path(), fs::path(commitDir) / filename, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                }
            }

            // Save Metadata
            std::ofstream meta(commitDir + "/meta.txt");
            meta << "id: " << nextId << "\n";
            meta << "message: " << message << "\n";
            meta << "timestamp: " << getTimestamp() << "\n";
            meta.close();

            updateHead(nextId);
            std::cout << "[commit_" << nextId << "] " << message << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error during commit: " << e.what() << std::endl;
        }
    }

    // 3. LOG: Formatted history output
    void showLog() {
        int current = getCurrentHead();
        if (current == 0) {
            std::cout << "No commit history found." << std::endl;
            return;
        }

        std::cout << "\n--- Commit History ---\n" << std::endl;
        for (int i = current; i > 0; --i) {
            std::string metaPath = ".mygit/commits/commit_" + std::to_string(i) + "/meta.txt";
            if (!fs::exists(metaPath)) continue;

            std::ifstream metaFile(metaPath);
            std::string line, id, msg, time;
            while (std::getline(metaFile, line)) {
                if (line.substr(0, 4) == "id: ") id = line.substr(4);
                else if (line.substr(0, 9) == "message: ") msg = line.substr(9);
                else if (line.substr(0, 11) == "timestamp: ") time = line.substr(11);
            }

            std::cout << "----------------------------------------" << std::endl;
            std::cout << "COMMIT ID: " << id << std::endl;
            std::cout << "DATE:      " << time << std::endl;
            std::cout << "MESSAGE:   " << msg << std::endl;
        }
        std::cout << "----------------------------------------\n" << std::endl;
    }

    // 4. CHECKOUT: Safe restoration
    void checkoutCommit(int commitId) {
        std::string commitDir = ".mygit/commits/commit_" + std::to_string(commitId);
        if (!fs::exists(commitDir)) {
            std::cerr << "Error: Commit " << commitId << " does not exist." << std::endl;
            return;
        }

        try {
            // Delete current files safely
            for (const auto& entry : fs::directory_iterator(".")) {
                if (!isIgnored(entry.path().filename().string())) {
                    fs::remove_all(entry.path());
                }
            }

            // Restore from snapshot
            for (const auto& entry : fs::directory_iterator(commitDir)) {
                if (entry.path().filename() != "meta.txt") {
                    fs::copy(entry.path(), "./" + entry.path().filename().string(), fs::copy_options::recursive);
                }
            }

            updateHead(commitId);
            std::cout << "Successfully reverted to commit " << commitId << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error during checkout: " << e.what() << std::endl;
        }
    }

    // 5. STATUS: Compare current workspace with last commit
    void showStatus() {
        int lastId = getCurrentHead();
        if (lastId == 0) {
            std::cout << "On initial state. No commits yet." << std::endl;
            return;
        }

        std::string lastDir = ".mygit/commits/commit_" + std::to_string(lastId);
        std::set<std::string> currentFiles;
        std::set<std::string> commitFiles;

        for (const auto& entry : fs::directory_iterator(".")) {
            if (!isIgnored(entry.path().filename().string())) {
                currentFiles.insert(entry.path().filename().string());
            }
        }
        for (const auto& entry : fs::directory_iterator(lastDir)) {
            if (entry.path().filename() != "meta.txt") {
                commitFiles.insert(entry.path().filename().string());
            }
        }

        std::cout << "\n--- Status ---\n" << std::endl;
        
        // New files
        for (const auto& file : currentFiles) {
            if (commitFiles.find(file) == commitFiles.end()) {
                std::cout << "  [NEW]      " << file << std::endl;
            } else {
                // Modified? (Check file sizes for a simple comparison)
                if (fs::file_size(file) != fs::file_size(lastDir + "/" + file)) {
                    std::cout << "  [MODIFIED] " << file << std::endl;
                }
            }
        }

        // Deleted files
        for (const auto& file : commitFiles) {
            if (currentFiles.find(file) == currentFiles.end()) {
                std::cout << "  [DELETED]  " << file << std::endl;
            }
        }
        std::cout << std::endl;
    }

private:
    bool isIgnored(const std::string& filename) {
        static const std::set<std::string> ignored = {".mygit", ".git", "mygit", "main.cpp", ".DS_Store"};
        return ignored.find(filename) != ignored.end() || filename.empty();
    }

    int getCurrentHead() {
        std::ifstream head(".mygit/HEAD");
        int id = 0;
        if (head >> id) return id;
        return 0;
    }

    void updateHead(int id) {
        std::ofstream head(".mygit/HEAD");
        head << id;
    }

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

int main(int argc, char* argv[]) {
    SimpleGit git;
    if (argc < 2) {
        std::cout << "Usage: mygit <init|commit|log|checkout|status> [args]" << std::endl;
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "init") {
        git.initRepo();
    } else if (cmd == "commit") {
        if (argc < 3) {
            std::cerr << "Usage: mygit commit \"message\"" << std::endl;
            return 1;
        }
        git.commitChanges(argv[2]);
    } else if (cmd == "log") {
        git.showLog();
    } else if (cmd == "checkout") {
        if (argc < 3) {
            std::cerr << "Usage: mygit checkout <id>" << std::endl;
            return 1;
        }
        git.checkoutCommit(std::stoi(argv[2]));
    } else if (cmd == "status") {
        git.showStatus();
    } else {
        std::cout << "Unknown command: " << cmd << std::endl;
    }

    return 0;
}
