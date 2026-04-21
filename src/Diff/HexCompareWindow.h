#pragma once
#include "FileMapping.h"
#include "HexCompareView.h"
#include <windows.h>
#include <string>

namespace npp {

// Singleton top-level window that memory-maps two files and renders them
// side-by-side via two HexCompareView children.
class HexCompareWindow
{
public:
    static void Open(HWND owner, HINSTANCE hInst,
                     const std::wstring& leftPath, const std::wstring& rightPath);

    static LRESULT CALLBACK Proc(HWND, UINT, WPARAM, LPARAM);

private:
    HexCompareWindow() = default;
    ~HexCompareWindow() = default;

    bool LoadFiles(const std::wstring& l, const std::wstring& r);
    void Layout();
    void UpdateStatus();
    void MirrorScroll(int src, int64_t topRow);

    HWND             hwnd_       = nullptr;
    HWND             statusBar_  = nullptr;
    HexCompareView   left_;
    HexCompareView   right_;
    ReadOnlyMapping  leftMap_;
    ReadOnlyMapping  rightMap_;
    std::wstring     leftPath_;
    std::wstring     rightPath_;
    bool             inMirror_   = false;

    static HexCompareWindow* s_active;
};

} // namespace npp
