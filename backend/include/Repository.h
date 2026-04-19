#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>


class Repository {
public:
    static bool init();
    static bool add(const std::string& filename);
    static bool commit(const std::string& message, const std::vector<std::string>& extraParents = {});
    static bool merge(const std::string& target);
    static void log();
    static bool branch(const std::string& branchName);
    static bool checkout(const std::string& branchName);
    static void diff(const std::string& commit1, const std::string& commit2);
    static void status();
    static bool tag(const std::string& tagName, const std::string& commitId = "");
    static bool stashPush();
    static bool stashPop();
    static bool reset(const std::string& commitId, bool hard = true);
    static std::string getGraph();
    
    // Merge Helpers
    static std::map<std::string, std::string> getTree(const std::string& commitId);
    static std::set<std::string> getHistory(const std::string& startId);
    static std::string findLCA(const std::string& id1, const std::string& id2);

private:
    static bool isIgnored(const std::string& path);
    static std::string calculateHash(const std::string& content);
    static std::string getHead();
    static void setHead(const std::string& value);
    static std::string getCurrentBranch();
    static std::string resolveId(const std::string& input);
    static std::vector<std::string> getAllBranches();
    static std::vector<std::string> getAllTags();
    static void storeObject(const std::string& hash, const std::string& content);
    static std::string getObject(const std::string& hash);
};

#endif
