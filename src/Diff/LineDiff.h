#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace npp {

enum class DiffOp : uint8_t {
    Equal,    // line present on both sides
    Add,      // present only on right
    Del,      // present only on left
    Change    // both sides have a line at this aligned position, but contents differ
};

struct DiffEntry {
    DiffOp op;
    int    leftLine;   // index into left lines, -1 if none
    int    rightLine;  // index into right lines, -1 if none
};

struct LineDiffOptions {
    bool ignoreWhitespace = false;  // strip all whitespace before comparing
    bool ignoreCase       = false;
};

// Split utf-8 text into lines (LF or CRLF or CR terminated). Trailing
// newline produces a final empty line, matching most diff tools.
std::vector<std::string> SplitLines(const std::string& utf8);

// Compute aligned diff between two line vectors. Result entries are in
// display order, suitable for line-by-line rendering on both sides.
std::vector<DiffEntry> ComputeLineDiff(const std::vector<std::string>& left,
                                       const std::vector<std::string>& right,
                                       const LineDiffOptions& opt = {});

} // namespace npp
