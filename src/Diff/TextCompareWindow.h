#pragma once
#include "LineDiff.h"
#include "../ScintillaComponent/ScintillaEditView.h"
#include <windows.h>
#include <string>
#include <vector>

namespace npp {

// Modeless top-level window that displays two read-only Scintilla editors
// side by side with line-level diff markers and synchronized vertical scroll.
//
// Lifetime: created with `new`, destroys self on WM_NCDESTROY.
class TextCompareWindow
{
public:
    // Open (or reuse) the singleton compare window. Loads `leftText` /
    // `rightText` (utf-8) into the two panes and recomputes the diff.
    static void Open(HWND owner, HINSTANCE hInst,
                     const std::wstring& leftTitle,  const std::string& leftText,
                     const std::wstring& rightTitle, const std::string& rightText,
                     const LineDiffOptions& opt);

    // Move caret to next/previous diff block in the active compare window.
    // No-op if there is none. Returns true if it handled the request.
    static bool NextDiffActive();
    static bool PreviousDiffActive();

    static LRESULT CALLBACK Proc(HWND, UINT, WPARAM, LPARAM);

private:
    TextCompareWindow() = default;
    ~TextCompareWindow() = default;

    void Load(const std::wstring& leftTitle,  const std::string& leftText,
              const std::wstring& rightTitle, const std::string& rightText,
              const LineDiffOptions& opt);
    void ApplyDiff();
    void Layout();
    void OnScroll(int srcView);
    void NextDiff(bool forward);
    void RecomputeFromCheckboxes();

    HWND              hwnd_       = nullptr;
    HWND              statusBar_  = nullptr;
    HWND              chkIgnoreWS_   = nullptr;
    HWND              chkIgnoreCase_ = nullptr;
    ScintillaEditView left_;
    ScintillaEditView right_;
    bool              inMirror_   = false;

    std::vector<DiffEntry> diff_;
    std::vector<int>       diffBlockLines_[2];  // first line of each diff block per side

    std::string       leftBytes_, rightBytes_;
    std::wstring      leftTitle_, rightTitle_;
    LineDiffOptions   opt_;

    static TextCompareWindow* s_active;
};

} // namespace npp
