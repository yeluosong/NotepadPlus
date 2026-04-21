#include "LineDiff.h"
#include <algorithm>
#include <cctype>

namespace npp {

std::vector<std::string> SplitLines(const std::string& s)
{
    std::vector<std::string> out;
    size_t i = 0, start = 0;
    while (i < s.size()) {
        char c = s[i];
        if (c == '\r' || c == '\n') {
            out.emplace_back(s.data() + start, i - start);
            if (c == '\r' && i + 1 < s.size() && s[i + 1] == '\n') ++i;
            ++i;
            start = i;
        } else {
            ++i;
        }
    }
    out.emplace_back(s.data() + start, s.size() - start);
    return out;
}

namespace {

std::string Normalize(const std::string& line, const LineDiffOptions& opt)
{
    std::string out;
    out.reserve(line.size());
    for (char c : line) {
        if (opt.ignoreWhitespace && (c == ' ' || c == '\t')) continue;
        if (opt.ignoreCase && c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
        out.push_back(c);
    }
    return out;
}

}  // namespace

std::vector<DiffEntry> ComputeLineDiff(const std::vector<std::string>& a,
                                       const std::vector<std::string>& b,
                                       const LineDiffOptions& opt)
{
    const int n = static_cast<int>(a.size());
    const int m = static_cast<int>(b.size());

    std::vector<std::string> na(n), nb(m);
    for (int i = 0; i < n; ++i) na[i] = Normalize(a[i], opt);
    for (int j = 0; j < m; ++j) nb[j] = Normalize(b[j], opt);

    // LCS DP. O(n*m) — caller should cap input size.
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            dp[i][j] = (na[i] == nb[j]) ? dp[i + 1][j + 1] + 1
                                        : std::max(dp[i + 1][j], dp[i][j + 1]);
        }
    }

    // Walk back: classify into Equal/Add/Del; later collapse adjacent Del+Add into Change.
    std::vector<DiffEntry> raw;
    raw.reserve(static_cast<size_t>(n + m));
    int i = 0, j = 0;
    while (i < n && j < m) {
        if (na[i] == nb[j]) {
            raw.push_back({DiffOp::Equal, i, j});
            ++i; ++j;
        } else if (dp[i + 1][j] >= dp[i][j + 1]) {
            raw.push_back({DiffOp::Del, i, -1});
            ++i;
        } else {
            raw.push_back({DiffOp::Add, -1, j});
            ++j;
        }
    }
    while (i < n) raw.push_back({DiffOp::Del, i++, -1});
    while (j < m) raw.push_back({DiffOp::Add, -1, j++});

    // Collapse runs: pair Del+Add into Change rows so left/right stay aligned.
    std::vector<DiffEntry> out;
    out.reserve(raw.size());
    size_t k = 0;
    while (k < raw.size()) {
        if (raw[k].op == DiffOp::Del) {
            size_t delStart = k, delEnd = k;
            while (delEnd < raw.size() && raw[delEnd].op == DiffOp::Del) ++delEnd;
            size_t addStart = delEnd, addEnd = delEnd;
            while (addEnd < raw.size() && raw[addEnd].op == DiffOp::Add) ++addEnd;
            size_t paired = std::min(delEnd - delStart, addEnd - addStart);
            for (size_t p = 0; p < paired; ++p) {
                out.push_back({DiffOp::Change,
                               raw[delStart + p].leftLine,
                               raw[addStart + p].rightLine});
            }
            for (size_t p = paired; p < delEnd - delStart; ++p)
                out.push_back(raw[delStart + p]);
            for (size_t p = paired; p < addEnd - addStart; ++p)
                out.push_back(raw[addStart + p]);
            k = addEnd;
        } else if (raw[k].op == DiffOp::Add) {
            // Standalone add (no preceding del run).
            out.push_back(raw[k++]);
        } else {
            out.push_back(raw[k++]);
        }
    }
    return out;
}

} // namespace npp
