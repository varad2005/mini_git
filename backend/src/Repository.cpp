#include "Repository.h"
#include "Diff.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <set>
#include <map>

namespace fs = std::filesystem;

bool Repository::init() {
    if (fs::exists(".mygit")) {
        std::cout << "Repository already initialized." << std::endl;
        return false;
    }

    try {
        fs::create_directory(".mygit");
        fs::create_directory(".mygit/commits");
        fs::create_directory(".mygit/objects");
        fs::create_directory(".mygit/branches");

        std::ofstream headFile(".mygit/HEAD");
        headFile << "ref: master";
        headFile.close();

        std::ofstream masterFile(".mygit/branches/master");
        masterFile << ""; 
        masterFile.close();

        std::ofstream indexFile(".mygit/index");
        indexFile.close();

        std::cout << "Initialized empty MyGit repository in " << fs::current_path() << "/.mygit/" << std::endl;
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

bool Repository::add(const std::string& filename) {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return false;
    }

    if (!fs::exists(filename)) {
        std::cerr << "Error: File '" << filename << "' does not exist." << std::endl;
        return false;
    }

    try {
        std::ifstream file(filename);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        std::string hash = calculateHash(content);

        storeObject(hash, content);

        // Update index: filename hash
        std::vector<std::pair<std::string, std::string>> indexEntries;
        std::ifstream indexIn(".mygit/index");
        std::string line;
        bool found = false;
        while (std::getline(indexIn, line)) {
            std::stringstream ss(line);
            std::string f, h;
            ss >> f >> h;
            if (f == filename) {
                indexEntries.push_back({f, hash});
                found = true;
            } else {
                indexEntries.push_back({f, h});
            }
        }
        indexIn.close();
        if (!found) indexEntries.push_back({filename, hash});

        std::ofstream indexOut(".mygit/index");
        for (const auto& entry : indexEntries) {
            indexOut << entry.first << " " << entry.second << "\n";
        }
        indexOut.close();

        std::cout << "Added " << filename << " to staging area." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

std::string Repository::getHead() {
    std::ifstream headFile(".mygit/HEAD");
    std::string line;
    std::getline(headFile, line);
    if (line.substr(0, 5) == "ref: ") {
        std::string branch = line.substr(5);
        std::ifstream branchFile(".mygit/branches/" + branch);
        std::string commitId;
        std::getline(branchFile, commitId);
        return commitId;
    }
    return line;
}

std::string Repository::getCurrentBranch() {
    std::ifstream headFile(".mygit/HEAD");
    std::string line;
    std::getline(headFile, line);
    if (line.substr(0, 5) == "ref: ") {
        return line.substr(5);
    }
    return "";
}

std::string Repository::calculateHash(const std::string& content) {
    unsigned long hash = 5381;
    for (char c : content) {
        hash = ((hash << 5) + hash) + c;
    }
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << hash;
    return ss.str();
}

bool Repository::commit(const std::string& message) {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return false;
    }

    if (fs::file_size(".mygit/index") == 0) {
        std::cout << "Nothing to commit, staging area is empty." << std::endl;
        return false;
    }

    std::string parentId = getHead();
    std::string timestamp = std::to_string(std::time(nullptr));
    std::string commitId = calculateHash(message + parentId + timestamp);

    std::string commitDir = ".mygit/commits/" + commitId;
    fs::create_directory(commitDir);

    // Copy index to commit's tree.txt
    fs::copy_file(".mygit/index", commitDir + "/tree.txt");

    // Save metadata
    std::ofstream metaFile(commitDir + "/metadata.txt");
    metaFile << "message: " << message << "\n";
    metaFile << "parent: " << parentId << "\n";
    metaFile << "timestamp: " << timestamp << "\n";
    metaFile.close();

    // Update branch pointer
    std::string branch = getCurrentBranch();
    if (!branch.empty()) {
        std::ofstream branchFile(".mygit/branches/" + branch);
        branchFile << commitId;
        branchFile.close();
    } else {
        std::ofstream headFile(".mygit/HEAD");
        headFile << commitId;
        headFile.close();
    }

    std::cout << "[" << (branch.empty() ? "detached HEAD" : branch) << " " << commitId << "] " << message << std::endl;
    return true;
}

void Repository::log() {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return;
    }

    std::string currentCommit = getHead();
    while (!currentCommit.empty() && currentCommit != "null" && currentCommit != "") {
        std::string metaPath = ".mygit/commits/" + currentCommit + "/metadata.txt";
        if (!fs::exists(metaPath)) break;

        std::ifstream metaFile(metaPath);
        std::string line;
        std::string message, parent, timestamp;

        while (std::getline(metaFile, line)) {
            if (line.substr(0, 9) == "message: ") message = line.substr(9);
            else if (line.substr(0, 8) == "parent: ") parent = line.substr(8);
            else if (line.substr(0, 11) == "timestamp: ") timestamp = line.substr(11);
        }

        std::cout << "commit " << currentCommit << std::endl;
        std::cout << "Date: " << timestamp << std::endl;
        std::cout << "\n    " << message << "\n" << std::endl;

        currentCommit = parent;
    }
}

bool Repository::branch(const std::string& branchName) {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return false;
    }

    std::string branchPath = ".mygit/branches/" + branchName;
    if (fs::exists(branchPath)) {
        std::cerr << "Error: Branch '" << branchName << "' already exists." << std::endl;
        return false;
    }

    std::string currentCommit = getHead();
    std::ofstream branchFile(branchPath);
    branchFile << currentCommit;
    branchFile.close();

    std::cout << "Created branch '" << branchName << "' at commit " << currentCommit << std::endl;
    return true;
}

bool Repository::checkout(const std::string& branchName) {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return false;
    }

    std::string commitId = resolveId(branchName);
    if (commitId.empty()) {
        std::cerr << "Error: Branch or commit '" << branchName << "' not found." << std::endl;
        return false;
    }

    // Check if it's a branch for the HEAD reference
    std::string branchPath = ".mygit/branches/" + branchName;
    if (fs::exists(branchPath)) {
        std::ofstream headFile(".mygit/HEAD");
        headFile << "ref: " << branchName;
        headFile.close();
        std::cout << "Switched to branch '" << branchName << "'" << std::endl;
    } else {
        std::ofstream headFile(".mygit/HEAD");
        headFile << commitId;
        headFile.close();
        std::cout << "Switched to detached HEAD at " << commitId << std::endl;
    }

    if (!commitId.empty() && commitId != "null" && commitId != "") {
        std::string treePath = ".mygit/commits/" + commitId + "/tree.txt";
        if (fs::exists(treePath)) {
            std::ifstream treeFile(treePath);
            std::string line;
            while (std::getline(treeFile, line)) {
                std::stringstream ss(line);
                std::string filename, hash;
                ss >> filename >> hash;
                std::string content = getObject(hash);
                std::ofstream outFile(filename);
                outFile << content;
                outFile.close();
            }
        }
    }

    return true;
}

void Repository::diff(const std::string& commit1, const std::string& commit2) {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return;
    }

    std::string id1 = resolveId(commit1);
    std::string id2 = resolveId(commit2);

    if (id1.empty() || id2.empty()) {
        std::cerr << "Error: One or both commit IDs/branches are invalid." << std::endl;
        return;
    }

    std::string path1 = ".mygit/commits/" + id1 + "/tree.txt";
    std::string path2 = ".mygit/commits/" + id2 + "/tree.txt";

    if (!fs::exists(path1) || !fs::exists(path2)) {
        std::cerr << "Error: One or both commit data snapshots are missing." << std::endl;
        return;
    }

    std::map<std::string, std::string> files1, files2;
    std::set<std::string> allFiles;

    std::ifstream f1(path1);
    std::string line;
    while (std::getline(f1, line)) {
        std::stringstream ss(line);
        std::string filename, hash;
        ss >> filename >> hash;
        files1[filename] = hash;
        allFiles.insert(filename);
    }

    std::ifstream f2(path2);
    while (std::getline(f2, line)) {
        std::stringstream ss(line);
        std::string filename, hash;
        ss >> filename >> hash;
        files2[filename] = hash;
        allFiles.insert(filename);
    }

    for (const auto& filename : allFiles) {
        if (files1[filename] == files2[filename]) continue;

        std::cout << "diff --mygit a/" << filename << " b/" << filename << std::endl;
        
        std::vector<std::string> lines1, lines2;
        if (files1.count(filename)) {
            std::stringstream ss(getObject(files1[filename]));
            std::string l;
            while (std::getline(ss, l)) lines1.push_back(l);
        }
        if (files2.count(filename)) {
            std::stringstream ss(getObject(files2[filename]));
            std::string l;
            while (std::getline(ss, l)) lines2.push_back(l);
        }

        Diff::compare(lines1, lines2);
    }
}

void Repository::storeObject(const std::string& hash, const std::string& content) {
    std::string path = ".mygit/objects/" + hash;
    if (!fs::exists(path)) {
        std::ofstream file(path);
        file << content;
        file.close();
    }
}

std::string Repository::resolveId(const std::string& input) {
    if (input.empty()) return "";

    // 1. Check if it's a branch
    std::string branchPath = ".mygit/branches/" + input;
    if (fs::exists(branchPath)) {
        std::ifstream branchFile(branchPath);
        std::string commitId;
        std::getline(branchFile, commitId);
        return commitId;
    }

    // 2. Check if it's a full ID or a prefix
    std::vector<std::string> matches;
    if (fs::exists(".mygit/commits")) {
        for (const auto& entry : fs::directory_iterator(".mygit/commits")) {
            std::string name = entry.path().filename().string();
            if (name.compare(0, input.length(), input) == 0) { // Starts with
                matches.push_back(name);
            }
        }
    }

    if (matches.size() == 1) {
        return matches[0];
    } else if (matches.size() > 1) {
        std::cerr << "Error: Commit ID '" << input << "' is ambiguous. Did you mean:\n";
        for (const auto& m : matches) std::cerr << "  " << m << "\n";
        return "";
    }

    return "";
}

std::string Repository::getObject(const std::string& hash) {
    std::ifstream file(".mygit/objects/" + hash);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> Repository::getAllBranches() {
    std::vector<std::string> branches;
    for (const auto& entry : fs::directory_iterator(".mygit/branches")) {
        branches.push_back(entry.path().filename().string());
    }
    return branches;
}

std::string Repository::getGraph() {
    std::stringstream ss;
    ss << "[\n";
    bool first = true;

    std::set<std::string> visited;
    std::vector<std::string> queue;

    // Start from all branches
    for (const auto& branch : getAllBranches()) {
        std::ifstream branchFile(".mygit/branches/" + branch);
        std::string commitId;
        std::getline(branchFile, commitId);
        if (!commitId.empty() && visited.find(commitId) == visited.end()) {
            queue.push_back(commitId);
        }
    }

    // Also check HEAD
    std::string headCommit = getHead();
    if (!headCommit.empty() && visited.find(headCommit) == visited.end()) {
        queue.push_back(headCommit);
    }

    // Breadth-first search to find all commits
    int head = 0;
    while(head < queue.size()){
        std::string currentCommit = queue[head++];
        if(visited.count(currentCommit)) continue;
        visited.insert(currentCommit);

        std::string metaPath = ".mygit/commits/" + currentCommit + "/metadata.txt";
        if (!fs::exists(metaPath)) continue;

        std::ifstream metaFile(metaPath);
        std::string line;
        std::string message, parent, timestamp;
        while (std::getline(metaFile, line)) {
            if (line.substr(0, 9) == "message: ") message = line.substr(9);
            else if (line.substr(0, 8) == "parent: ") parent = line.substr(8);
            else if (line.substr(0, 11) == "timestamp: ") timestamp = line.substr(11);
        }

        if (!first) ss << ",\n";
        ss << "  {\n";
        ss << "    \"id\": \"" << currentCommit << "\",\n";
        ss << "    \"parent\": \"" << parent << "\",\n";
        ss << "    \"message\": \"" << message << "\",\n";
        ss << "    \"timestamp\": \"" << timestamp << "\",\n";
        ss << "    \"branches\": [";
        
        bool firstBranch = true;
        for (const auto& branch : getAllBranches()) {
            std::ifstream bf(".mygit/branches/" + branch);
            std::string bc; std::getline(bf, bc);
            if (bc == currentCommit) {
                if (!firstBranch) ss << ", ";
                ss << "\"" << branch << "\"";
                firstBranch = false;
            }
        }
        ss << "]\n";
        ss << "  }";
        first = false;

        if (!parent.empty() && parent != "null" && visited.find(parent) == visited.end()) {
            queue.push_back(parent);
        }
    }

    ss << "\n]";
    return ss.str();
}
