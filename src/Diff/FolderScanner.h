#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace npp {

enum class FolderDiffStatus : uint8_t {
    Same,         // both sides have file with identical bytes
    Different,    // both sides have file but bytes differ (or sizes differ)
    LeftOnly,     // exists only in left tree
    RightOnly,    // exists only in right tree
    DirOnly,      // a directory entry that exists on both sides (no leaf compare)
};

struct FolderEntry {
    std::wstring relPath;     // relative path from the root, with backslashes
    int          depth = 0;   // number of '\' in relPath — used by the view for indenting
    bool         isDir = false;
    bool         leftExists  = false;
    bool         rightExists = false;
    uint64_t     leftSize   = 0;
    uint64_t     rightSize  = 0;
    FILETIME     leftMtime{};
    FILETIME     rightMtime{};
    FolderDiffStatus status = FolderDiffStatus::Same;
};

struct FolderScanOptions {
    bool recursive = true;
    // 200 MiB byte-compare cap per file. Above this we assume Different
    // unless sizes match — keeps the scan responsive on large trees.
    uint64_t maxByteCompare = 200ULL * 1024 * 1024;
};

// Walks the two roots and returns one entry per relative path found in either
// tree, sorted by relPath case-insensitive. Files identical on both sides
// have status==Same; everything else is flagged.
std::vector<FolderEntry> ScanFolders(const std::wstring& leftRoot,
                                     const std::wstring& rightRoot,
                                     const FolderScanOptions& opt = {});

} // namespace npp
