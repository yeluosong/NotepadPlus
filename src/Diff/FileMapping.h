#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

namespace npp {

// Read-only memory-mapped view of a file. Move-only RAII wrapper.
// Empty on error — caller checks Data() != nullptr.
class ReadOnlyMapping
{
public:
    ReadOnlyMapping() = default;
    ~ReadOnlyMapping() { Close(); }

    ReadOnlyMapping(const ReadOnlyMapping&) = delete;
    ReadOnlyMapping& operator=(const ReadOnlyMapping&) = delete;

    ReadOnlyMapping(ReadOnlyMapping&& o) noexcept { Steal(o); }
    ReadOnlyMapping& operator=(ReadOnlyMapping&& o) noexcept
    {
        if (this != &o) { Close(); Steal(o); }
        return *this;
    }

    // Open `path` and map it entirely. On failure returns false and sets
    // `errorOut` (if non-null) to a human-readable message.
    bool Open(const std::wstring& path, std::wstring* errorOut = nullptr);
    void Close();

    const uint8_t* Data() const { return data_; }
    uint64_t       Size() const { return size_; }

private:
    void Steal(ReadOnlyMapping& o)
    {
        file_ = o.file_; map_ = o.map_; data_ = o.data_; size_ = o.size_;
        o.file_ = INVALID_HANDLE_VALUE; o.map_ = nullptr; o.data_ = nullptr; o.size_ = 0;
    }

    HANDLE   file_ = INVALID_HANDLE_VALUE;
    HANDLE   map_  = nullptr;
    uint8_t* data_ = nullptr;
    uint64_t size_ = 0;
};

} // namespace npp
