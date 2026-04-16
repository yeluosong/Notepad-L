#pragma once
#include <string>
#include <vector>

namespace npp {

enum class DiffOp : unsigned char { Equal, Add, Remove, Change };

struct DiffEntry {
    DiffOp op   = DiffOp::Equal;
    int    a    = -1;   // 0-based line index in left  (or -1 for pure Add)
    int    b    = -1;   // 0-based line index in right (or -1 for pure Remove)
};

// Diff two line vectors into an aligned op list.
// Pairs adjacent Remove+Add (single-line) into Change for nicer rendering.
std::vector<DiffEntry> ComputeLineDiff(const std::vector<std::string>& left,
                                       const std::vector<std::string>& right);

// Split a UTF-8 buffer into lines (without the line terminators).
std::vector<std::string> SplitLines(const std::string& body);

} // namespace npp
