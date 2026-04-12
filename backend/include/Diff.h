#ifndef DIFF_H
#define DIFF_H

#include <vector>
#include <string>

class Diff {
public:
    static void compare(const std::vector<std::string>& lines1, const std::vector<std::string>& lines2);
};

#endif
