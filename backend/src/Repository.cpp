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

    std::vector<std::string> filesToAdd;
    if (filename == ".") {
        for (const auto& entry : fs::recursive_directory_iterator(".")) {
            if (entry.is_regular_file() && !isIgnored(entry.path().string())) {
                filesToAdd.push_back(fs::relative(entry.path(), ".").string());
            }
        }
    } else {
        if (!fs::exists(filename)) {
            std::cerr << "Error: File '" << filename << "' does not exist." << std::endl;
            return false;
        }
        if (isIgnored(filename)) {
            std::cout << "File '" << filename << "' is ignored." << std::endl;
            return false;
        }
        filesToAdd.push_back(filename);
    }

    try {
        std::map<std::string, std::string> indexEntries;
        std::ifstream indexIn(".mygit/index");
        std::string line;
        while (std::getline(indexIn, line)) {
            std::stringstream ss(line);
            std::string f, h;
            ss >> f >> h;
            if (!f.empty()) indexEntries[f] = h;
        }
        indexIn.close();

        for (const auto& f : filesToAdd) {
            std::ifstream file(f);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            std::string hash = calculateHash(content);
            storeObject(hash, content);
            indexEntries[f] = hash;
            if (filesToAdd.size() <= 10) {
                std::cout << "Added " << f << " to staging area." << std::endl;
            }
        }
        if (filesToAdd.size() > 10) {
            std::cout << "Added " << filesToAdd.size() << " files to staging area." << std::endl;
        }

        std::ofstream indexOut(".mygit/index");
        for (const auto& entry : indexEntries) {
            indexOut << entry.first << " " << entry.second << "\n";
        }
        indexOut.close();

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

void Repository::setHead(const std::string& value) {
    std::ofstream headFile(".mygit/HEAD");
    if (fs::exists(".mygit/branches/" + value)) {
        headFile << "ref: " << value;
    } else {
        headFile << value;
    }
    headFile.close();
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

bool Repository::commit(const std::string& message, const std::vector<std::string>& extraParents) {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return false;
    }

    if (fs::file_size(".mygit/index") == 0) {
        std::cout << "Nothing to commit, staging area is empty." << std::endl;
        return false;
    }

    std::string primaryParent = getHead();
    std::string timestamp = std::to_string(std::time(nullptr));
    
    // Hash includes all parents
    std::string parentsStr = primaryParent;
    for (const auto& p : extraParents) parentsStr += "," + p;
    std::string commitId = calculateHash(message + parentsStr + timestamp);

    std::string commitDir = ".mygit/commits/" + commitId;
    fs::create_directory(commitDir);

    // Copy index to commit's tree.txt
    fs::copy_file(".mygit/index", commitDir + "/tree.txt");

    // Save metadata
    std::ofstream metaFile(commitDir + "/metadata.txt");
    metaFile << "message: " << message << "\n";
    metaFile << "parent: " << primaryParent << "\n";
    for (const auto& p : extraParents) {
        metaFile << "parent: " << p << "\n";
    }
    metaFile << "timestamp: " << timestamp << "\n";
    metaFile.close();
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

        std::time_t t = std::stoll(timestamp);
        std::tm* tm_ptr = std::localtime(&t);
        char date_str[100];
        std::strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_ptr);

        std::cout << "\033[33mcommit " << currentCommit << "\033[0m" << std::endl;
        std::cout << "Date: " << date_str << std::endl;
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

    // Identify current tracked files (from current HEAD) BEFORE we update it
    std::set<std::string> currentTrackedFiles;
    std::string previousHead = getHead();
    if (!previousHead.empty() && previousHead != "null") {
        std::ifstream treeFile(".mygit/commits/" + previousHead + "/tree.txt");
        std::string line;
        while (std::getline(treeFile, line)) {
            std::stringstream ss(line);
            std::string f, h;
            ss >> f >> h;
            if (!f.empty()) currentTrackedFiles.insert(f);
        }
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
            std::set<std::string> targetTrackedFiles;
            std::ifstream treeFile(treePath);
            std::string line;
            while (std::getline(treeFile, line)) {
                std::stringstream ss(line);
                std::string filename, hash;
                ss >> filename >> hash;
                if (filename.empty()) continue;
                targetTrackedFiles.insert(filename);
                
                std::string content = getObject(hash);
                if (!fs::path(filename).parent_path().empty()) {
                    fs::create_directories(fs::path(filename).parent_path());
                }
                std::ofstream outFile(filename);
                outFile << content;
                outFile.close();
            }
            treeFile.close();

            // Remove files that were tracked but are NOT in the target commit
            for (const auto& f : currentTrackedFiles) {
                if (targetTrackedFiles.find(f) == targetTrackedFiles.end()) {
                    if (fs::exists(f)) {
                        fs::remove(f);
                        std::cout << "Removed " << f << " (not in target commit)" << std::endl;
                    }
                }
            }

            // Update index to match checkout
            fs::copy_file(treePath, ".mygit/index", fs::copy_options::overwrite_existing);
        }
    }

    return true;
}

void Repository::status() {
    if (!fs::exists(".mygit")) {
        std::cerr << "Error: Not a mygit repository." << std::endl;
        return;
    }

    std::map<std::string, std::string> indexEntries;
    std::ifstream indexIn(".mygit/index");
    std::string line;
    while (std::getline(indexIn, line)) {
        std::stringstream ss(line);
        std::string f, h;
        ss >> f >> h;
        if (!f.empty()) indexEntries[f] = h;
    }
    indexIn.close();

    std::map<std::string, std::string> headEntries;
    std::string currentHead = getHead();
    if (!currentHead.empty() && currentHead != "null") {
        std::ifstream treeFile(".mygit/commits/" + currentHead + "/tree.txt");
        while (std::getline(treeFile, line)) {
            std::stringstream ss(line);
            std::string f, h;
            ss >> f >> h;
            if (!f.empty()) headEntries[f] = h;
        }
    }

    std::cout << "On branch \033[1;36m" << (getCurrentBranch().empty() ? "detached HEAD" : getCurrentBranch()) << "\033[0m" << std::endl;
    std::cout << std::endl;

    // 1. Changes to be committed (Index vs HEAD)
    std::cout << "\033[32mChanges to be committed:\033[0m" << std::endl;
    bool hasStaged = false;
    std::set<std::string> allTracked;
    for (auto const& [f, h] : indexEntries) allTracked.insert(f);
    for (auto const& [f, h] : headEntries) allTracked.insert(f);

    for (const auto& f : allTracked) {
        if (indexEntries.count(f) && !headEntries.count(f)) {
            std::cout << "\033[32m  (new file)  " << f << "\033[0m" << std::endl;
            hasStaged = true;
        } else if (!indexEntries.count(f) && headEntries.count(f)) {
            std::cout << "\033[32m  (deleted)   " << f << "\033[0m" << std::endl;
            hasStaged = true;
        } else if (indexEntries[f] != headEntries[f]) {
            std::cout << "\033[32m  (modified)  " << f << "\033[0m" << std::endl;
            hasStaged = true;
        }
    }
    if (!hasStaged) std::cout << "  (none)" << std::endl;
    std::cout << std::endl;

    // 2. Changes not staged for commit (Workspace vs Index)
    std::cout << "\033[31mChanges not staged for commit:\033[0m" << std::endl;
    bool hasUnstaged = false;
    for (auto const& [f, h] : indexEntries) {
        if (!fs::exists(f)) {
            std::cout << "\033[31m  (deleted)   " << f << "\033[0m" << std::endl;
            hasUnstaged = true;
        } else {
            std::ifstream file(f);
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (calculateHash(buffer.str()) != h) {
                std::cout << "\033[31m  (modified)  " << f << "\033[0m" << std::endl;
                hasUnstaged = true;
            }
        }
    }
    if (!hasUnstaged) std::cout << "  (none)" << std::endl;
    std::cout << std::endl;

    // 3. Untracked files
    std::cout << "\033[31mUntracked files:\033[0m" << std::endl;
    bool hasUntracked = false;
    for (const auto& entry : fs::recursive_directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string relPath = fs::relative(entry.path(), ".").string();
            if (!isIgnored(relPath) && !indexEntries.count(relPath)) {
                std::cout << "\033[31m  " << relPath << "\033[0m" << std::endl;
                hasUntracked = true;
            }
        }
    }
    if (!hasUntracked) std::cout << "  (none)" << std::endl;
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

    // 1.5 Check if it's a tag
    std::string tagPath = ".mygit/tags/" + input;
    if (fs::exists(tagPath)) {
        std::ifstream tagFile(tagPath);
        std::string commitId;
        std::getline(tagFile, commitId);
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
    if (fs::exists(".mygit/branches")) {
        for (const auto& entry : fs::directory_iterator(".mygit/branches")) {
            branches.push_back(entry.path().filename().string());
        }
    }
    return branches;
}

std::vector<std::string> Repository::getAllTags() {
    std::vector<std::string> tags;
    if (fs::exists(".mygit/tags")) {
        for (const auto& entry : fs::directory_iterator(".mygit/tags")) {
            tags.push_back(entry.path().filename().string());
        }
    }
    return tags;
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
        ss << "],\n";
        ss << "    \"tags\": [";
        bool firstTag = true;
        for (const auto& tag : getAllTags()) {
            std::ifstream tf(".mygit/tags/" + tag);
            std::string tc; std::getline(tf, tc);
            if (tc == currentCommit) {
                if (!firstTag) ss << ", ";
                ss << "\"" << tag << "\"";
                firstTag = false;
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

bool Repository::isIgnored(const std::string& path) {
    if (path == "." || path == "..") return true;

    // Load patterns from .mygitignore if it exists
    static std::set<std::string> patterns;
    static bool loaded = false;
    if (!loaded) {
        if (fs::exists(".mygitignore")) {
            std::ifstream ignoreFile(".mygitignore");
            std::string p;
            while (std::getline(ignoreFile, p)) {
                if (!p.empty() && p[0] != '#') patterns.insert(p);
            }
        }
        loaded = true;
    }
    
    fs::path p(path);
    for (const auto& part : p) {
        std::string partStr = part.string();
        static const std::set<std::string> hardcoded = {
            ".mygit", ".git", "mygit", "Diff.o", "Repository.o", "main.o", ".venv", "graphify-out", ".agent", "backend", "simple-git-snapshot"
        };
        if (hardcoded.count(partStr)) return true;
        if (patterns.count(partStr)) return true;
        
        // Simple wildcard support: *.o
        for (const auto& pattern : patterns) {
            if (pattern.size() > 1 && pattern[0] == '*' && pattern[1] == '.') {
                std::string ext = pattern.substr(1); // e.g. ".o"
                if (partStr.size() >= ext.size() && partStr.substr(partStr.size() - ext.size()) == ext) return true;
            }
        }

        if (partStr.size() > 0 && partStr[0] == '.') return true;
    }

    return false;
}

bool Repository::tag(const std::string& tagName, const std::string& commitId) {
    if (!fs::exists(".mygit/tags")) fs::create_directories(".mygit/tags");
    
    std::string id = commitId.empty() ? getHead() : resolveId(commitId);
    if (id.empty()) {
        std::cerr << "\033[31mError: Cannot resolve commit ID for tag.\033[0m" << std::endl;
        return false;
    }

    std::ofstream tagFile(".mygit/tags/" + tagName);
    tagFile << id;
    std::cout << "\033[32mCreated tag '" << tagName << "' at commit " << id.substr(0, 8) << "\033[0m" << std::endl;
    return true;
}

bool Repository::stashPush() {
    if (!fs::exists(".mygit/stash")) fs::create_directories(".mygit/stash");
    
    // Simplest stack implementation: numbered stashes
    int nextId = 0;
    while (fs::exists(".mygit/stash/stash_" + std::to_string(nextId))) nextId++;
    
    std::string stashDir = ".mygit/stash/stash_" + std::to_string(nextId);
    fs::create_directories(stashDir);
    
    // Save current index
    if (fs::exists(".mygit/index")) {
        fs::copy(".mygit/index", stashDir + "/index");
    }
    
    // Save modified workspace files relative to root
    for (auto const& entry : fs::recursive_directory_iterator(".")) {
        std::string path = entry.path().lexically_normal().string();
        if (path.size() > 2 && path.substr(0, 2) == "./") path = path.substr(2);
        
        if (isIgnored(path) || fs::is_directory(path)) continue;
        
        // Only save if modified compared to index (or staging it anyway)
        // For simplicity in this demo, let's just copy everything to a backup
        std::string backupPath = stashDir + "/files/" + path;
        fs::create_directories(fs::path(backupPath).parent_path());
        fs::copy(path, backupPath);
    }
    
    std::cout << "\033[32mSaved working directory and index state WIP on stash@{" << nextId << "}\033[0m" << std::endl;
    return true;
}

bool Repository::stashPop() {
    if (!fs::exists(".mygit/stash")) return false;
    
    int lastId = -1;
    while (fs::exists(".mygit/stash/stash_" + std::to_string(lastId + 1))) lastId++;
    
    if (lastId == -1) {
        std::cout << "\033[33mNo stash entries found.\033[0m" << std::endl;
        return false;
    }
    
    std::string stashDir = ".mygit/stash/stash_" + std::to_string(lastId);
    
    // Restore index
    if (fs::exists(stashDir + "/index")) {
        fs::copy(stashDir + "/index", ".mygit/index", fs::copy_options::overwrite_existing);
    }
    
    // Restore files
    if (fs::exists(stashDir + "/files")) {
        for (auto const& entry : fs::recursive_directory_iterator(stashDir + "/files")) {
            if (fs::is_directory(entry)) continue;
            std::string relative = entry.path().lexically_relative(stashDir + "/files").string();
            
            fs::path pParent = fs::path(relative).parent_path();
            if (!pParent.empty()) {
                fs::create_directories(pParent);
            }
            fs::copy(entry.path(), relative, fs::copy_options::overwrite_existing);
        }
    }
    
    fs::remove_all(stashDir);
    std::cout << "\033[32mDropped stash@{" << lastId << "} and restored working state.\033[0m" << std::endl;
    return true;
}

bool Repository::reset(const std::string& commitId, bool hard) {
    std::string id = resolveId(commitId);
    if (id.empty()) {
        std::cerr << "\033[31mError: Could not resolve commit ID '" << commitId << "'\033[0m" << std::endl;
        return false;
    }
    
    // Move branch pointer
    std::string branch = getCurrentBranch();
    if (!branch.empty()) {
        std::ofstream branchFile(".mygit/branches/" + branch);
        branchFile << id;
    } else {
        setHead(id);
    }
    
    if (hard) {
        return checkout(id); // Use checkout logic to force workspace update
    }
    
    std::cout << "\033[32mHEAD is now at " << id.substr(0, 8) << "\033[0m" << std::endl;
    return true;
}

std::map<std::string, std::string> Repository::getTree(const std::string& commitId) {
    std::map<std::string, std::string> tree;
    std::string treePath = ".mygit/commits/" + commitId + "/tree.txt";
    if (!fs::exists(treePath)) return tree;
    
    std::ifstream file(treePath);
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string filename, hash;
        ss >> filename >> hash;
        if (!filename.empty()) tree[filename] = hash;
    }
    return tree;
}

std::set<std::string> Repository::getHistory(const std::string& startId) {
    std::set<std::string> history;
    if (startId.empty() || startId == "null") return history;
    
    std::vector<std::string> queue = {startId};
    unsigned int head = 0;
    while (head < queue.size()) {
        std::string curr = queue[head++];
        if (history.count(curr)) continue;
        history.insert(curr);
        
        std::string metaPath = ".mygit/commits/" + curr + "/metadata.txt";
        if (fs::exists(metaPath)) {
            std::ifstream file(metaPath);
            std::string line;
            while (std::getline(file, line)) {
                if (line.substr(0, 8) == "parent: ") {
                    std::string p = line.substr(8);
                    if (!p.empty() && p != "null") queue.push_back(p);
                }
            }
        }
    }
    return history;
}

std::string Repository::findLCA(const std::string& id1, const std::string& id2) {
    std::set<std::string> history1 = getHistory(id1);
    
    // BFS on id2 to find first common ancestor
    std::set<std::string> visited2;
    std::vector<std::string> queue = {id2};
    unsigned int head = 0;
    while (head < queue.size()) {
        std::string curr = queue[head++];
        if (visited2.count(curr)) continue;
        visited2.insert(curr);
        
        if (history1.count(curr)) return curr; // Found common ancestor
        
        std::string metaPath = ".mygit/commits/" + curr + "/metadata.txt";
        if (fs::exists(metaPath)) {
            std::ifstream file(metaPath);
            std::string line;
            while (std::getline(file, line)) {
                if (line.substr(0, 8) == "parent: ") {
                    std::string p = line.substr(8);
                    if (!p.empty() && p != "null") queue.push_back(p);
                }
            }
        }
    }
    return "";
}

bool Repository::merge(const std::string& targetInput) {
    std::string remoteId = resolveId(targetInput);
    if (remoteId.empty()) {
        std::cerr << "\033[31mError: Could not resolve target '" << targetInput << "'\033[0m" << std::endl;
        return false;
    }
    
    std::string localId = getHead();
    if (localId == remoteId) {
        std::cout << "Already up to date." << std::endl;
        return true;
    }
    
    std::string baseId = findLCA(localId, remoteId);
    
    if (baseId == remoteId) {
        std::cout << "Already up to date." << std::endl;
        return true;
    }
    
    if (baseId == localId) {
        std::cout << "Fast-forwarding to " << remoteId.substr(0, 8) << "..." << std::endl;
        return checkout(remoteId);
    }
    
    std::cout << "Auto-merging with base " << baseId.substr(0, 8) << "..." << std::endl;
    
    std::map<std::string, std::string> baseTree = getTree(baseId);
    std::map<std::string, std::string> localTree = getTree(localId);
    std::map<std::string, std::string> remoteTree = getTree(remoteId);
    
    std::set<std::string> allFiles;
    for (auto const& [f, h] : baseTree) allFiles.insert(f);
    for (auto const& [f, h] : localTree) allFiles.insert(f);
    for (auto const& [f, h] : remoteTree) allFiles.insert(f);
    
    bool conflictFound = false;
    std::map<std::string, std::string> mergeIndex;

    for (const auto& f : allFiles) {
        std::string hBase = baseTree.count(f) ? baseTree[f] : "";
        std::string hLocal = localTree.count(f) ? localTree[f] : "";
        std::string hRemote = remoteTree.count(f) ? remoteTree[f] : "";
        
        if (hLocal == hRemote) {
            if (!hLocal.empty()) mergeIndex[f] = hLocal;
        } else if (hBase == hLocal) {
            if (!hRemote.empty()) mergeIndex[f] = hRemote;
        } else if (hBase == hRemote) {
            if (!hLocal.empty()) mergeIndex[f] = hLocal;
        } else {
            // CONFLICT
            std::cout << "\033[31mCONFLICT (content): Merge conflict in " << f << "\033[0m" << std::endl;
            conflictFound = true;
            
            std::string contentLocal = hLocal.empty() ? "" : getObject(hLocal);
            std::string contentRemote = hRemote.empty() ? "" : getObject(hRemote);
            
            std::stringstream conflict;
            conflict << "<<<<<<<< HEAD\n" << contentLocal 
                     << "\n========\n" << contentRemote 
                     << "\n>>>>>>>> " << targetInput << "\n";
            
            std::string c = conflict.str();
            std::ofstream outFile(f);
            outFile << c;
            
            // For index, we'll store the conflict version for now
            std::string hConflict = calculateHash(c);
            storeObject(hConflict, c);
            mergeIndex[f] = hConflict;
        }
    }
    
    // Update index with merged state
    std::ofstream indexFile(".mygit/index");
    for (auto const& [f, h] : mergeIndex) {
        indexFile << f << " " << h << "\n";
        // Also update workspace
        if (h != (localTree.count(f) ? localTree[f] : "")) {
            std::string content = getObject(h);
            fs::path pParent = fs::path(f).parent_path();
            if (!pParent.empty()) {
                fs::create_directories(pParent);
            }
            std::ofstream fOut(f);
            fOut << content;
        }
    }
    indexFile.close();
    
    if (conflictFound) {
        std::cout << "Automatic merge failed; fix conflicts and then commit the result." << std::endl;
        return false;
    }
    
    std::string msg = "Merge branch '" + targetInput + "'";
    return commit(msg, {remoteId});
}
