#pragma once
#include <deque>
#include <string>

namespace npp {

// Global app parameters. Singleton. M1 scope: recent files + window placement.
class Parameters
{
public:
    static Parameters& Instance();

    // Most-recently-used files (front = most recent). Capped to kMaxRecent.
    static constexpr size_t kMaxRecent = 20;
    const std::deque<std::wstring>& RecentFiles() const { return recent_; }
    void AddRecent(const std::wstring& path);
    void ClearRecent();

    // Dark mode preference.
    bool DarkMode() const { return darkMode_; }
    void SetDarkMode(bool v) { darkMode_ = v; }

    // Load/save settings from %APPDATA%\NotePad-L\config.xml
    // M1 uses a trivial text format; replaced by TinyXML-2 in M2.
    void Load();
    void Save() const;

    std::wstring ConfigDir() const;

private:
    Parameters() = default;
    std::deque<std::wstring> recent_;
    bool darkMode_ = false;
    // Cache of the last bytes written to recent.txt; Save() skips the write
    // when the serialized state is unchanged. Matters for bulk operations
    // like multi-file open / save-all which call Save() per item.
    mutable std::string lastSavedBlob_;
    mutable std::string lastSavedSettings_;
};

} // namespace npp
