#include "Parameters.h"
#include "../MISC/Common/FileIO.h"
#include "../MISC/Common/StringUtil.h"

#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <sstream>

namespace npp {

Parameters& Parameters::Instance()
{
    static Parameters p;
    return p;
}

std::wstring Parameters::ConfigDir() const
{
    PWSTR appdata = nullptr;
    std::wstring dir;
    if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
        dir = appdata;
        ::CoTaskMemFree(appdata);
        dir += L"\\NotePad-L";
        ::CreateDirectoryW(dir.c_str(), nullptr);
    }
    return dir;
}

void Parameters::AddRecent(const std::wstring& path)
{
    auto it = std::find_if(recent_.begin(), recent_.end(),
        [&](const std::wstring& p) { return ::_wcsicmp(p.c_str(), path.c_str()) == 0; });
    if (it != recent_.end()) recent_.erase(it);
    recent_.push_front(path);
    while (recent_.size() > kMaxRecent) recent_.pop_back();
}

void Parameters::ClearRecent()
{
    recent_.clear();
}

void Parameters::Load()
{
    const std::wstring file = ConfigDir() + L"\\recent.txt";
    std::vector<char> bytes;
    if (ReadFileAll(file, bytes)) {
        std::wstring text = Utf8ToWide(std::string_view(bytes.data(), bytes.size()));
        std::wstringstream ss(text);
        std::wstring line;
        recent_.clear();
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            if (!line.empty()) recent_.push_back(line);
        }
        while (recent_.size() > kMaxRecent) recent_.pop_back();

        // Prime the save cache so the first Save() after Load() is a no-op when
        // nothing has changed in memory relative to what's already on disk.
        std::wstring primed;
        for (const auto& r : recent_) { primed += r; primed += L"\r\n"; }
        lastSavedBlob_ = WideToUtf8(primed);
    }

    // Load settings (dark mode, etc.)
    const std::wstring settingsFile = ConfigDir() + L"\\settings.txt";
    std::vector<char> sBytes;
    if (ReadFileAll(settingsFile, sBytes)) {
        std::wstring st = Utf8ToWide(std::string_view(sBytes.data(), sBytes.size()));
        std::wstringstream ss2(st);
        std::wstring line;
        while (std::getline(ss2, line)) {
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            if (line.rfind(L"theme=", 0) == 0) {
                int id = _wtoi(line.c_str() + 6);
                if (id >= 0 && id < static_cast<int>(ThemeId::Count_))
                    theme_ = static_cast<ThemeId>(id);
            }
            // Back-compat with earlier builds that only wrote darkMode=0/1.
            else if (line == L"darkMode=1") theme_ = ThemeId::DarkPro;
            else if (line == L"darkMode=0") theme_ = ThemeId::ModernLight;
            else if (line.rfind(L"cmpTextLeft=",   0) == 0) cmpTextLeft   = line.substr(12);
            else if (line.rfind(L"cmpTextRight=",  0) == 0) cmpTextRight  = line.substr(13);
            else if (line.rfind(L"cmpHexLeft=",    0) == 0) cmpHexLeft    = line.substr(11);
            else if (line.rfind(L"cmpHexRight=",   0) == 0) cmpHexRight   = line.substr(12);
            else if (line.rfind(L"cmpFolderLeft=", 0) == 0) cmpFolderLeft = line.substr(14);
            else if (line.rfind(L"cmpFolderRight=",0) == 0) cmpFolderRight= line.substr(15);
            else if (line == L"wordWrap=1") wordWrap_ = true;
            else if (line == L"wordWrap=0") wordWrap_ = false;
        }
    }
}

void Parameters::Save() const
{
    std::wstring text;
    for (const auto& r : recent_) { text += r; text += L"\r\n"; }
    std::string utf8 = WideToUtf8(text);
    if (utf8 != lastSavedBlob_) {
        const std::wstring file = ConfigDir() + L"\\recent.txt";
        if (WriteFileAtomic(file, utf8.data(), utf8.size()))
            lastSavedBlob_ = std::move(utf8);
    }

    // Save settings
    std::wstring settings;
    settings += L"theme=";
    settings += std::to_wstring(static_cast<int>(theme_));
    settings += L"\r\n";
    settings += L"wordWrap=";
    settings += (wordWrap_ ? L"1" : L"0");
    settings += L"\r\n";
    auto kv = [&](const wchar_t* key, const std::wstring& v) {
        if (v.empty()) return;
        settings += key; settings += L"="; settings += v; settings += L"\r\n";
    };
    kv(L"cmpTextLeft",    cmpTextLeft);
    kv(L"cmpTextRight",   cmpTextRight);
    kv(L"cmpHexLeft",     cmpHexLeft);
    kv(L"cmpHexRight",    cmpHexRight);
    kv(L"cmpFolderLeft",  cmpFolderLeft);
    kv(L"cmpFolderRight", cmpFolderRight);
    std::string sUtf8 = WideToUtf8(settings);
    if (sUtf8 != lastSavedSettings_) {
        const std::wstring sFile = ConfigDir() + L"\\settings.txt";
        if (WriteFileAtomic(sFile, sUtf8.data(), sUtf8.size()))
            lastSavedSettings_ = std::move(sUtf8);
    }
}

} // namespace npp
