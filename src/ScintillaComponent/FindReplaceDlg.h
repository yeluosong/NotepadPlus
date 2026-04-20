#pragma once
#include <windows.h>
#include <string>

namespace npp {

class ScintillaEditView;

enum class FindMode { Find, Replace };

class FindReplaceDlg
{
public:
    // Editor pointer must remain valid while the dialog is shown.
    void Show(HWND owner, HINSTANCE hInst, ScintillaEditView* view, FindMode mode);
    void Close();
    bool IsVisible() const { return hwnd_ != nullptr; }
    HWND Hwnd() const { return hwnd_; }

    // Driven by main frame accelerators: F3 / Shift+F3.
    bool FindNextAgain(ScintillaEditView* view);
    bool FindPrevAgain(ScintillaEditView* view);

private:
    enum : int {
        IDC_FIND_WHAT = 1001,
        IDC_REPLACE_WITH,
        IDC_CASE,
        IDC_WHOLEWORD,
        IDC_REGEX,
        IDC_WRAP,
        IDC_UPWARD,
        IDC_FIND_NEXT,
        IDC_FIND_PREV,
        IDC_COUNT,
        IDC_MARK_ALL,
        IDC_REPLACE_ONE,
        IDC_REPLACE_ALL,
        IDC_STATUS,
        IDC_QUICK_FIND,
        IDC_QUICK_REPLACE,
        IDC_REPLACE_LABEL,
        IDC_OPTIONS_GROUP,
    };

    static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR        HandleMessage(HWND, UINT, WPARAM, LPARAM);
    void           Layout(HWND);
    void           SetControlsForMode(HWND, FindMode);
    void           ReadOptions(HWND);

    bool           DoFind(ScintillaEditView* view, bool forward, bool reportWrap);
    int            DoReplaceAll(ScintillaEditView* view);
    int            DoMarkAll(ScintillaEditView* view);
    int            DoCount(ScintillaEditView* view);
    void           SetStatus(HWND, const std::wstring& msg);

    int  BuildSearchFlags() const;
    std::string GetFindUtf8(HWND) const;
    std::string GetReplaceUtf8(HWND) const;

    HWND hwnd_ = nullptr;
    FindMode mode_ = FindMode::Find;
    ScintillaEditView* view_ = nullptr;

    // Options cache (re-used by F3 after the dialog is closed).
    std::string lastFind_;
    std::string lastReplace_;
    bool optCase_  = false;
    bool optWhole_ = false;
    bool optRegex_ = false;
    bool optWrap_  = true;
    bool optUp_    = false;

    // True while the controls below the Replace row are slid up to
    // collapse the gap that the hidden Replace label/edit would leave.
    bool shiftedUp_ = false;
};

} // namespace npp
