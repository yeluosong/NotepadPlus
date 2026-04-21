#include "FileMapping.h"

namespace npp {

bool ReadOnlyMapping::Open(const std::wstring& path, std::wstring* errorOut)
{
    Close();
    file_ = ::CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_ == INVALID_HANDLE_VALUE) {
        if (errorOut) *errorOut = L"Cannot open file.";
        return false;
    }

    LARGE_INTEGER sz{};
    if (!::GetFileSizeEx(file_, &sz)) {
        if (errorOut) *errorOut = L"Cannot query file size.";
        Close();
        return false;
    }
    size_ = static_cast<uint64_t>(sz.QuadPart);
    if (size_ == 0) {
        // Empty file — keep handle but leave data_ null; Data()/Size() reflect that.
        return true;
    }

    map_ = ::CreateFileMappingW(file_, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!map_) {
        if (errorOut) *errorOut = L"CreateFileMapping failed.";
        Close();
        return false;
    }
    data_ = static_cast<uint8_t*>(::MapViewOfFile(map_, FILE_MAP_READ, 0, 0, 0));
    if (!data_) {
        if (errorOut) *errorOut = L"MapViewOfFile failed.";
        Close();
        return false;
    }
    return true;
}

void ReadOnlyMapping::Close()
{
    if (data_) { ::UnmapViewOfFile(data_); data_ = nullptr; }
    if (map_)  { ::CloseHandle(map_); map_ = nullptr; }
    if (file_ != INVALID_HANDLE_VALUE) { ::CloseHandle(file_); file_ = INVALID_HANDLE_VALUE; }
    size_ = 0;
}

} // namespace npp
