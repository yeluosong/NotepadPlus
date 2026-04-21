#pragma once
#include "FolderScanner.h"
#include <windows.h>
#include <functional>
#include <vector>

namespace npp {

// Self-drawn dual-pane folder compare list, modelled after Beyond Compare:
//   [ left:  indent + name | size | mtime ] [status gutter] [ right: ... ]
// Files that exist on only one side render with a blank pane on the other
// side (greyed out). Different files get a yellow background on both sides.
// Selection, vertical scroll, keyboard navigation, and a double-click
// activation callback are supported.
class FolderCompareView
{
public:
    bool Create(HWND parent, HINSTANCE hInst, int id);
    HWND Hwnd() const { return hwnd_; }

    void SetEntries(std::vector<FolderEntry> entries);
    int  Selected() const { return selected_; }
    const FolderEntry* SelectedEntry() const {
        if (selected_ < 0 || selected_ >= static_cast<int>(visible_.size())) return nullptr;
        return &entries_[visible_[selected_]];
    }

    void SetOnActivate(std::function<void(int)> cb) { onActivate_ = std::move(cb); }

    void Invalidate();

    static LRESULT CALLBACK StaticProc(HWND, UINT, WPARAM, LPARAM);

    // Light defaults — caller can override before Create.
    COLORREF bg_         = RGB(0xFF, 0xFF, 0xFF);
    COLORREF fg_         = RGB(0x1E, 0x1E, 0x1E);
    COLORREF bgHeader_   = RGB(0xF0, 0xF0, 0xF0);
    COLORREF bgAlt_      = RGB(0xFA, 0xFA, 0xFA);
    COLORREF bgSelect_   = RGB(0xCC, 0xE4, 0xF7);
    COLORREF bgDiff_     = RGB(0xFF, 0xF0, 0x99);
    COLORREF bgMissing_  = RGB(0xEC, 0xEC, 0xEC);   // empty pane for one-side-only
    COLORREF fgMuted_    = RGB(0x88, 0x88, 0x88);
    COLORREF fgGutter_   = RGB(0x40, 0x40, 0x40);
    COLORREF fgAdd_      = RGB(0x1B, 0x80, 0x1B);
    COLORREF fgDel_      = RGB(0xC0, 0x32, 0x32);
    COLORREF colSep_     = RGB(0xC8, 0xC8, 0xC8);

private:
    LRESULT Proc(HWND, UINT, WPARAM, LPARAM);

    void EnsureFont();
    void Paint(HDC dc);
    void PaintPane(HDC dc, const RECT& pane, const FolderEntry& e,
                   bool isLeft, COLORREF rowBg, int triangleX);
    void UpdateScrollbar();
    void EnsureVisible(int row);
    void OnVScroll(WPARAM w);
    void ScrollBy(int rows);
    int  RowAtY(int y) const;
    int  RowsVisible() const;

    void RebuildVisible();
    void ToggleEntry(int entryIdx);
    // True if `x` (client coords) lands on the expand/collapse triangle of
    // the visible row at index `visIdx` (assumed to be a directory).
    bool HitTriangle(int visIdx, int x) const;
    int  TriangleX(int paneLeft, int depth) const;

    HWND  hwnd_     = nullptr;
    HFONT font_     = nullptr;
    int   rowH_     = 0;
    int   charW_    = 0;
    int   topRow_   = 0;
    int   selected_ = -1;     // index into visible_, not entries_

    std::vector<FolderEntry> entries_;
    std::vector<bool>        expanded_;   // per entries_ index; true for non-dirs
    std::vector<int>         visible_;    // entries_ indices currently shown
    std::function<void(int)> onActivate_;
};

} // namespace npp
