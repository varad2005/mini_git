#include "../include/Diff.h"
#include <iostream>
#include <vector>
#include <algorithm>

void Diff::compare(const std::vector<std::string>& lines1, const std::vector<std::string>& lines2) {
    int m = lines1.size();
    int n = lines2.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));

    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (lines1[i - 1] == lines2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    struct DiffLine {
        char type;
        std::string content;
        int l1, l2;
    };

    int i = m, j = n;
    std::vector<DiffLine> diffs;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && lines1[i - 1] == lines2[j - 1]) {
            diffs.push_back({' ', lines1[i - 1], i, j});
            i--; j--;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            diffs.push_back({'+', lines2[j - 1], i, j});
            j--;
        } else {
            diffs.push_back({'-', lines1[i - 1], i, j});
            i--;
        }
    }
    std::reverse(diffs.begin(), diffs.end());

    // Print with hunks
    if (diffs.empty()) return;

    bool inHunk = false;
    for (size_t k = 0; k < diffs.size(); ++k) {
        bool isChange = (diffs[k].type != ' ');
        
        // Simple hunk logic: if it's a change, print it. 
        // For a 1st year student, let's just group contiguous changes and a bit of context.
        if (isChange && !inHunk) {
            std::cout << "\033[36m@@ hunk start near line " << diffs[k].l1 << " @@\033[0m" << std::endl;
            inHunk = true;
        } else if (!isChange) {
            inHunk = false;
        }

        if (diffs[k].type == '+') std::cout << "\033[32m+ " << diffs[k].content << "\033[0m" << std::endl;
        else if (diffs[k].type == '-') std::cout << "\033[31m- " << diffs[k].content << "\033[0m" << std::endl;
        else if (isChange || inHunk) std::cout << "  " << diffs[k].content << std::endl; 
        // For brevity, we could skip long stretches of unchanged lines, but let's keep it simple for now as requested.
    }
}
