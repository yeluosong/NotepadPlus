#include "FolderScanner.h"
#include "FileMapping.h"
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <unordered_map>

namespace npp {

namespace {

struct SideEntry {
    bool     exists = false;
    bool     isDir  = false;
    uint64_t size   = 0;
    FILETIME mtime{};
};

// Case-insensitive key for the merge map. Windows file system is case
// preserving but case insensitive, so "Foo.txt" on left and "foo.txt" on
// right are the same file.
struct CIHash {
    size_t operator()(const std::wstring& s) const noexcept {
        size_t h = 1469598103934665603ULL;
        for (wchar_t c : s) {
            h ^= static_cast<size_t>(std::towlower(c));
            h *= 1099511628211ULL;
        }
        return h;
    }
};
struct CIEq {
    bool operator()(const std::wstring& a, const std::wstring& b) const noexcept {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::towlower(a[i]) != std::towlower(b[i])) return false;
        return true;
    }
};

void WalkDir(const std::wstring& root,
             const std::wstring& rel,
             bool recursive,
             std::unordered_map<std::wstring, SideEntry, CIHash, CIEq>& out)
{
    std::wstring search = root;
    if (!rel.empty()) { search += L"\\"; search += rel; }
    search += L"\\*";

    WIN32_FIND_DATAW fd;
    HANDLE h = ::FindFirstFileW(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.cFileName[0] == L'.' &&
            (fd.cFileName[1] == 0 ||
             (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0)))
            continue;

        std::wstring childRel = rel.empty()
            ? std::wstring(fd.cFileName)
            : rel + L"\\" + fd.cFileName;

        SideEntry e;
        e.exists = true;
        e.isDir  = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        e.mtime  = fd.ftLastWriteTime;
        if (!e.isDir) {
            ULARGE_INTEGER sz; sz.LowPart = fd.nFileSizeLow; sz.HighPart = fd.nFileSizeHigh;
            e.size = sz.QuadPart;
        }
        out[childRel] = e;

        if (e.isDir && recursive) {
            WalkDir(root, childRel, recursive, out);
        }
    } while (::FindNextFileW(h, &fd));
    ::FindClose(h);
}

bool BytesEqual(const std::wstring& a, const std::wstring& b, uint64_t cap)
{
    ReadOnlyMapping ma, mb;
    if (!ma.Open(a) || !mb.Open(b)) return false;
    if (ma.Size() != mb.Size()) return false;
    if (ma.Size() > cap) return false;     // bail out — caller treats as "Different"
    if (ma.Size() == 0) return true;
    return std::memcmp(ma.Data(), mb.Data(), static_cast<size_t>(ma.Size())) == 0;
}

}  // namespace

std::vector<FolderEntry> ScanFolders(const std::wstring& leftRoot,
                                     const std::wstring& rightRoot,
                                     const FolderScanOptions& opt)
{
    std::unordered_map<std::wstring, SideEntry, CIHash, CIEq> L, R;
    WalkDir(leftRoot,  L"", opt.recursive, L);
    WalkDir(rightRoot, L"", opt.recursive, R);

    // Union of keys, preserving the casing from whichever side has it.
    std::unordered_map<std::wstring, FolderEntry, CIHash, CIEq> merged;
    for (auto& kv : L) {
        FolderEntry e;
        e.relPath     = kv.first;
        e.isDir       = kv.second.isDir;
        e.leftExists  = true;
        e.leftSize    = kv.second.size;
        e.leftMtime   = kv.second.mtime;
        merged[kv.first] = e;
    }
    for (auto& kv : R) {
        auto it = merged.find(kv.first);
        if (it == merged.end()) {
            FolderEntry e;
            e.relPath     = kv.first;
            e.isDir       = kv.second.isDir;
            e.rightExists = true;
            e.rightSize   = kv.second.size;
            e.rightMtime  = kv.second.mtime;
            merged[kv.first] = e;
        } else {
            it->second.rightExists = true;
            it->second.rightSize   = kv.second.size;
            it->second.rightMtime  = kv.second.mtime;
            // If one side is a dir and the other a file, prefer the dir flag
            // from whichever side it's on — but mark Different.
            if (it->second.isDir != kv.second.isDir) {
                it->second.isDir = false;  // treat as conflicting leaf
            }
        }
    }

    std::vector<FolderEntry> out;
    out.reserve(merged.size());
    for (auto& kv : merged) out.push_back(std::move(kv.second));

    // Classify status.
    auto join = [](const std::wstring& root, const std::wstring& rel) {
        return root + L"\\" + rel;
    };
    for (auto& e : out) {
        if (e.isDir) {
            e.status = (e.leftExists && e.rightExists)
                ? FolderDiffStatus::DirOnly
                : (e.leftExists ? FolderDiffStatus::LeftOnly : FolderDiffStatus::RightOnly);
            continue;
        }
        if (!e.leftExists)       { e.status = FolderDiffStatus::RightOnly; continue; }
        if (!e.rightExists)      { e.status = FolderDiffStatus::LeftOnly;  continue; }
        if (e.leftSize != e.rightSize) { e.status = FolderDiffStatus::Different; continue; }
        if (e.leftSize == 0)          { e.status = FolderDiffStatus::Same; continue; }
        e.status = BytesEqual(join(leftRoot, e.relPath), join(rightRoot, e.relPath), opt.maxByteCompare)
            ? FolderDiffStatus::Same
            : FolderDiffStatus::Different;
    }

    // Lexical case-insensitive sort interleaves a dir with its contents in
    // the natural tree order (e.g. "Asm" -> "Asm\arm" -> "Asm\arm\foo.s").
    std::sort(out.begin(), out.end(), [](const FolderEntry& a, const FolderEntry& b) {
        const wchar_t* x = a.relPath.c_str();
        const wchar_t* y = b.relPath.c_str();
        while (*x && *y) {
            wchar_t cx = std::towlower(*x++);
            wchar_t cy = std::towlower(*y++);
            if (cx != cy) return cx < cy;
        }
        return *x == 0 && *y != 0;
    });

    for (auto& e : out) {
        int d = 0;
        for (wchar_t c : e.relPath) if (c == L'\\') ++d;
        e.depth = d;
    }

    return out;
}

} // namespace npp
