#include "LineDiff.h"
#include <algorithm>

namespace npp {

std::vector<std::string> SplitLines(const std::string& body)
{
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos <= body.size()) {
        size_t nl = body.find('\n', pos);
        size_t end = (nl == std::string::npos) ? body.size() : nl;
        std::string line = body.substr(pos, end - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(std::move(line));
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
    return out;
}

namespace {

// Standard LCS table (O(n*m) memory) — fine for typical compare sizes.
std::vector<std::vector<int>> BuildLcs(const std::vector<std::string>& a,
                                       const std::vector<std::string>& b)
{
    int n = static_cast<int>(a.size());
    int m = static_cast<int>(b.size());
    std::vector<std::vector<int>> t(n + 1, std::vector<int>(m + 1, 0));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a[i-1] == b[j-1]) t[i][j] = t[i-1][j-1] + 1;
            else t[i][j] = std::max(t[i-1][j], t[i][j-1]);
        }
    }
    return t;
}

} // namespace

std::vector<DiffEntry> ComputeLineDiff(const std::vector<std::string>& a,
                                       const std::vector<std::string>& b)
{
    auto t = BuildLcs(a, b);
    std::vector<DiffEntry> rev;
    int i = static_cast<int>(a.size());
    int j = static_cast<int>(b.size());
    while (i > 0 && j > 0) {
        if (a[i-1] == b[j-1]) {
            rev.push_back({ DiffOp::Equal, i - 1, j - 1 });
            --i; --j;
        } else if (t[i-1][j] >= t[i][j-1]) {
            rev.push_back({ DiffOp::Remove, i - 1, -1 });
            --i;
        } else {
            rev.push_back({ DiffOp::Add, -1, j - 1 });
            --j;
        }
    }
    while (i > 0) { rev.push_back({ DiffOp::Remove, --i, -1 }); }
    while (j > 0) { rev.push_back({ DiffOp::Add,    -1, --j }); }
    std::reverse(rev.begin(), rev.end());

    // Coalesce single Remove+Add pairs into Change entries.
    std::vector<DiffEntry> out;
    out.reserve(rev.size());
    for (size_t k = 0; k < rev.size(); ++k) {
        if (k + 1 < rev.size() &&
            rev[k].op == DiffOp::Remove && rev[k+1].op == DiffOp::Add)
        {
            out.push_back({ DiffOp::Change, rev[k].a, rev[k+1].b });
            ++k;
        } else {
            out.push_back(rev[k]);
        }
    }
    return out;
}

} // namespace npp
