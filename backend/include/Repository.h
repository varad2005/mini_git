#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class Repository {
public:
    static bool init();
    static bool add(const std::string& filename);
    static bool commit(const std::string& message);
    static void log();
    static bool branch(const std::string& branchName);
    static bool checkout(const std::string& branchName);
    static void diff(const std::string& commit1, const std::string& commit2);
    static std::string getGraph();

private:
    static std::string calculateHash(const std::string& content);
    static std::string getHead();
    static void setHead(const std::string& value);
    static std::string getCurrentBranch();
    static std::vector<std::string> getAllBranches();
    static void storeObject(const std::string& hash, const std::string& content);
    static std::string getObject(const std::string& hash);
};

#endif
