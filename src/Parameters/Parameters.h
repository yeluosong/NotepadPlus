#pragma once
#include <deque>
#include <string>

namespace npp {

// Named UI themes. Keep values stable — persisted to settings.txt.
enum class ThemeId : int {
    ModernLight  = 0,  // 现代清爽
    DarkPro      = 1,  // 暗夜专业
    HighContrast = 2,  // 简洁高对比
    Mint         = 3,  // 柔和护眼
    Nordic       = 4,  // 北欧极简
    DeepBlue     = 5,  // 代码风格

    Count_
};

constexpr bool IsDarkFamily(ThemeId t) {
    return t == ThemeId::DarkPro || t == ThemeId::DeepBlue;
}

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

    // Active theme.
    ThemeId Theme() const { return theme_; }
    void SetTheme(ThemeId t) { theme_ = t; }

    // Convenience: "is this theme dark-family?" — kept for call sites that only
    // need a light/dark split (DWM immersive dark, markdown accent colors).
    bool DarkMode() const { return IsDarkFamily(theme_); }

    // Load/save settings from %APPDATA%\NotePad-L\config.xml
    // M1 uses a trivial text format; replaced by TinyXML-2 in M2.
    // Last-used paths for the three Compare pickers — restored on next launch.
    std::wstring cmpTextLeft;
    std::wstring cmpTextRight;
    std::wstring cmpHexLeft;
    std::wstring cmpHexRight;
    std::wstring cmpFolderLeft;
    std::wstring cmpFolderRight;

    void Load();
    void Save() const;

    std::wstring ConfigDir() const;

private:
    Parameters() = default;
    std::deque<std::wstring> recent_;
    ThemeId theme_ = ThemeId::ModernLight;
    // Cache of the last bytes written to recent.txt; Save() skips the write
    // when the serialized state is unchanged. Matters for bulk operations
    // like multi-file open / save-all which call Save() per item.
    mutable std::string lastSavedBlob_;
    mutable std::string lastSavedSettings_;
};

} // namespace npp
