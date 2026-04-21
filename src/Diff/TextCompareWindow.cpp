#include "TextCompareWindow.h"
#include "../Parameters/Parameters.h"
#include "../Parameters/Stylers.h"
#include "../resource.h"
#include <Scintilla.h>
#include <commctrl.h>
#include <algorithm>
#include <cstring>

namespace npp {

TextCompareWindow* TextCompareWindow::s_active = nullptr;

namespace {
constexpr wchar_t kClass[]    = L"NotePadLTextCompareWnd";
constexpr int    kMarkerAdd    = 20;
constexpr int    kMarkerDel    = 21;
constexpr int    kMarkerChange = 22;
constexpr int    kMarkerEmpty  = 23;   // padding row that has no source line
constexpr int    kToolbarH     = 28;
constexpr int    kStatusH      = 22;
constexpr int    kSplitW       = 4;

// Diff highlighting: every changed/added/removed line gets the same red
// background — the user wants "different = red" without a per-op palette.
// `kColEmpty` is the inert grey used to pad the missing side so line numbers
// stay aligned. Two palettes (light/dark) chosen via the active theme.
struct DiffPalette { COLORREF diff; COLORREF empty; };
DiffPalette CurrentDiffPalette()
{
    if (Parameters::Instance().DarkMode())
        return { RGB(0x70, 0x60, 0x20), RGB(0x2E, 0x2E, 0x2E) };
    return     { RGB(0xFF, 0xF0, 0x99), RGB(0xE8, 0xE8, 0xE8) };
}

void RegisterClassOnce(HINSTANCE hInst)
{
    static bool reg = false;
    if (reg) return;
    WNDCLASSW wc{};
    wc.lpfnWndProc   = TextCompareWindow::Proc;
    wc.hInstance     = hInst;
    wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClass;
    ::RegisterClassW(&wc);
    reg = true;
}

void SetupEditor(ScintillaEditView& v)
{
    // Apply the active theme/lexer (plain text). Sets fonts, fg/bg, caret,
    // selection, line-number margin colors — i.e. matches the main editor.
    ApplyLanguage(v, LangType::Text);
    v.Call(SCI_SETREADONLY, 1);

    // Diff markers (background only — keeps the margin clean).
    auto pal = CurrentDiffPalette();
    auto def = [&](int id, COLORREF col) {
        v.Call(SCI_MARKERDEFINE,  static_cast<uptr_t>(id), SC_MARK_BACKGROUND);
        v.Call(SCI_MARKERSETBACK, static_cast<uptr_t>(id), col);
    };
    def(kMarkerAdd,    pal.diff);
    def(kMarkerDel,    pal.diff);
    def(kMarkerChange, pal.diff);
    def(kMarkerEmpty,  pal.empty);
}

}  // namespace

void TextCompareWindow::Open(HWND owner, HINSTANCE hInst,
                             const std::wstring& leftTitle,  const std::string& leftText,
                             const std::wstring& rightTitle, const std::string& rightText,
                             const LineDiffOptions& opt)
{
    RegisterClassOnce(hInst);
    if (!s_active) {
        TextCompareWindow* self = new TextCompareWindow();
        s_active = self;
        HWND hw = ::CreateWindowExW(0, kClass, L"Text Compare",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1100, 700,
            owner, nullptr, hInst, self);
        if (!hw) {
            delete self;
            s_active = nullptr;
            return;
        }
    }
    s_active->Load(leftTitle, leftText, rightTitle, rightText, opt);
    ::ShowWindow(s_active->hwnd_, SW_SHOW);
    ::SetForegroundWindow(s_active->hwnd_);
}

bool TextCompareWindow::NextDiffActive()
{
    if (!s_active || !s_active->hwnd_) return false;
    s_active->NextDiff(true);
    return true;
}

bool TextCompareWindow::PreviousDiffActive()
{
    if (!s_active || !s_active->hwnd_) return false;
    s_active->NextDiff(false);
    return true;
}

LRESULT CALLBACK TextCompareWindow::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    TextCompareWindow* self = nullptr;
    if (m == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(l);
        self = static_cast<TextCompareWindow*>(cs->lpCreateParams);
        self->hwnd_ = h;
        ::SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<TextCompareWindow*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }

    if (!self) return ::DefWindowProcW(h, m, w, l);

    HINSTANCE hInst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(h, GWLP_HINSTANCE));

    switch (m) {
    case WM_CREATE: {
        ::InitCommonControls();
        self->statusBar_ = ::CreateWindowExW(0, STATUSCLASSNAMEW, L"",
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
            h, reinterpret_cast<HMENU>(static_cast<INT_PTR>(9001)), hInst, nullptr);

        // Toolbar area: two checkboxes + Next/Prev buttons.
        self->chkIgnoreWS_ = ::CreateWindowExW(0, L"BUTTON", L"Ignore whitespace",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            8, 6, 130, 18, h,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(1001)), hInst, nullptr);
        self->chkIgnoreCase_ = ::CreateWindowExW(0, L"BUTTON", L"Ignore case",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            142, 6, 100, 18, h,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(1002)), hInst, nullptr);
        ::CreateWindowExW(0, L"BUTTON", L"Prev (Shift+F7)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            252, 4, 110, 22, h,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(1003)), hInst, nullptr);
        ::CreateWindowExW(0, L"BUTTON", L"Next (F7)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            366, 4, 110, 22, h,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(1004)), hInst, nullptr);

        self->left_.Create(h, hInst);
        self->right_.Create(h, hInst);
        SetupEditor(self->left_);
        SetupEditor(self->right_);
        return 0;
    }
    case WM_SIZE:
        self->Layout();
        if (self->statusBar_) ::SendMessageW(self->statusBar_, WM_SIZE, 0, 0);
        return 0;
    case WM_ERASEBKGND: {
        const auto& pal = Ui();
        HDC dc = reinterpret_cast<HDC>(w);
        RECT rc; ::GetClientRect(h, &rc);
        HBRUSH br = ::CreateSolidBrush(pal.chromeBg);
        ::FillRect(dc, &rc, br);
        ::DeleteObject(br);
        return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        const auto& pal = Ui();
        HDC dc = reinterpret_cast<HDC>(w);
        ::SetTextColor(dc, pal.text);
        ::SetBkColor  (dc, pal.chromeBg);
        static HBRUSH s_br = nullptr;
        static COLORREF s_for = 0;
        if (!s_br || s_for != pal.chromeBg) {
            if (s_br) ::DeleteObject(s_br);
            s_br = ::CreateSolidBrush(pal.chromeBg);
            s_for = pal.chromeBg;
        }
        return reinterpret_cast<LRESULT>(s_br);
    }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == 1001 || id == 1002) {
            if (HIWORD(w) == BN_CLICKED) self->RecomputeFromCheckboxes();
        } else if (id == 1003) {
            self->NextDiff(false);
        } else if (id == 1004) {
            self->NextDiff(true);
        }
        return 0;
    }
    case WM_NOTIFY: {
        auto* nm = reinterpret_cast<NMHDR*>(l);
        if (!nm) break;
        auto* scn = reinterpret_cast<SCNotification*>(l);
        if (scn->nmhdr.code == SCN_UPDATEUI) {
            int srcView = (nm->hwndFrom == self->left_.Hwnd())  ? 0
                        : (nm->hwndFrom == self->right_.Hwnd()) ? 1 : -1;
            if (srcView >= 0) self->OnScroll(srcView);
        }
        return 0;
    }
    case WM_KEYDOWN: {
        if (w == VK_F7) {
            self->NextDiff((::GetKeyState(VK_SHIFT) & 0x8000) == 0);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        ::DestroyWindow(h);
        return 0;
    case WM_NCDESTROY:
        if (s_active == self) s_active = nullptr;
        delete self;
        return 0;
    }
    return ::DefWindowProcW(h, m, w, l);
}

void TextCompareWindow::Layout()
{
    if (!hwnd_) return;
    RECT rc; ::GetClientRect(hwnd_, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int top = kToolbarH;
    int bottom = h - kStatusH;
    int paneH = bottom - top;
    int leftW = (w - kSplitW) / 2;
    int rightX = leftW + kSplitW;
    int rightW = w - rightX;
    RECT lr{ 0,      top, leftW,            top + paneH };
    RECT rr{ rightX, top, rightX + rightW,  top + paneH };
    left_.Resize(lr);
    right_.Resize(rr);
}

void TextCompareWindow::Load(const std::wstring& lt, const std::string& lb,
                             const std::wstring& rt, const std::string& rb,
                             const LineDiffOptions& opt)
{
    leftTitle_  = lt;
    rightTitle_ = rt;
    leftBytes_  = lb;
    rightBytes_ = rb;
    opt_        = opt;

    if (chkIgnoreWS_)   ::SendMessageW(chkIgnoreWS_,   BM_SETCHECK, opt.ignoreWhitespace ? BST_CHECKED : BST_UNCHECKED, 0);
    if (chkIgnoreCase_) ::SendMessageW(chkIgnoreCase_, BM_SETCHECK, opt.ignoreCase       ? BST_CHECKED : BST_UNCHECKED, 0);

    std::wstring caption = L"Text Compare — " + lt + L"  ↔  " + rt;
    ::SetWindowTextW(hwnd_, caption.c_str());

    ApplyDiff();
}

void TextCompareWindow::RecomputeFromCheckboxes()
{
    opt_.ignoreWhitespace = ::SendMessageW(chkIgnoreWS_,   BM_GETCHECK, 0, 0) == BST_CHECKED;
    opt_.ignoreCase       = ::SendMessageW(chkIgnoreCase_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    ApplyDiff();
}

void TextCompareWindow::ApplyDiff()
{
    auto leftLines  = SplitLines(leftBytes_);
    auto rightLines = SplitLines(rightBytes_);

    constexpr size_t kMaxLines = 20000;
    if (leftLines.size() > kMaxLines || rightLines.size() > kMaxLines) {
        ::MessageBoxW(hwnd_,
            L"File too large for line-diff (limit 20,000 lines per side).",
            L"Compare", MB_OK | MB_ICONWARNING);
        return;
    }

    diff_ = ComputeLineDiff(leftLines, rightLines, opt_);

    // Build aligned text for both panes: each diff entry occupies one row.
    // For Add rows, left side is blank padding; for Del rows, right is blank.
    std::string leftView, rightView;
    leftView.reserve(leftBytes_.size() + diff_.size());
    rightView.reserve(rightBytes_.size() + diff_.size());

    struct RowMark { int row; int marker; };
    std::vector<RowMark> leftMarks, rightMarks;
    diffBlockLines_[0].clear();
    diffBlockLines_[1].clear();

    int row = 0;
    int diffs = 0;
    bool inBlock = false;

    auto append = [](std::string& dst, const std::string& s, bool addNL) {
        dst.append(s);
        if (addNL) dst.push_back('\n');
    };

    for (size_t k = 0; k < diff_.size(); ++k) {
        const auto& e = diff_[k];
        bool isLast = (k == diff_.size() - 1);
        const std::string& l = (e.leftLine  >= 0) ? leftLines[e.leftLine]   : std::string();
        const std::string& r = (e.rightLine >= 0) ? rightLines[e.rightLine] : std::string();
        append(leftView,  l, !isLast);
        append(rightView, r, !isLast);

        switch (e.op) {
        case DiffOp::Equal: inBlock = false; break;
        case DiffOp::Add:
            rightMarks.push_back({row, kMarkerAdd});
            leftMarks .push_back({row, kMarkerEmpty});
            if (!inBlock) { diffBlockLines_[0].push_back(row); diffBlockLines_[1].push_back(row); ++diffs; inBlock = true; }
            break;
        case DiffOp::Del:
            leftMarks .push_back({row, kMarkerDel});
            rightMarks.push_back({row, kMarkerEmpty});
            if (!inBlock) { diffBlockLines_[0].push_back(row); diffBlockLines_[1].push_back(row); ++diffs; inBlock = true; }
            break;
        case DiffOp::Change:
            leftMarks .push_back({row, kMarkerChange});
            rightMarks.push_back({row, kMarkerChange});
            if (!inBlock) { diffBlockLines_[0].push_back(row); diffBlockLines_[1].push_back(row); ++diffs; inBlock = true; }
            break;
        }
        ++row;
    }

    auto loadInto = [](ScintillaEditView& v, const std::string& bytes,
                       const std::vector<RowMark>& marks) {
        v.Call(SCI_SETREADONLY, 0);
        v.Call(SCI_CLEARALL);
        v.Call(SCI_ADDTEXT,
               static_cast<uptr_t>(bytes.size()),
               reinterpret_cast<sptr_t>(bytes.data()));
        v.Call(SCI_SETREADONLY, 1);
        for (const auto& m : marks)
            v.Call(SCI_MARKERADD, static_cast<uptr_t>(m.row),
                   static_cast<sptr_t>(m.marker));
    };
    loadInto(left_,  leftView,  leftMarks);
    loadInto(right_, rightView, rightMarks);

    wchar_t buf[64];
    ::swprintf_s(buf, L"%d differences", diffs);
    if (statusBar_) ::SetWindowTextW(statusBar_, buf);
}

void TextCompareWindow::OnScroll(int srcView)
{
    if (inMirror_) return;
    inMirror_ = true;
    ScintillaEditView& src   = (srcView == 0) ? left_  : right_;
    ScintillaEditView& other = (srcView == 0) ? right_ : left_;
    int first = static_cast<int>(src.Call(SCI_GETFIRSTVISIBLELINE));
    other.Call(SCI_SETFIRSTVISIBLELINE, static_cast<uptr_t>(first));
    inMirror_ = false;
}

void TextCompareWindow::NextDiff(bool forward)
{
    if (diffBlockLines_[0].empty()) return;
    int curLine = static_cast<int>(left_.Call(SCI_LINEFROMPOSITION,
        static_cast<uptr_t>(left_.Call(SCI_GETCURRENTPOS))));
    int target = -1;
    if (forward) {
        for (int b : diffBlockLines_[0]) if (b > curLine) { target = b; break; }
        if (target < 0) target = diffBlockLines_[0].front();  // wrap
    } else {
        for (auto it = diffBlockLines_[0].rbegin(); it != diffBlockLines_[0].rend(); ++it)
            if (*it < curLine) { target = *it; break; }
        if (target < 0) target = diffBlockLines_[0].back();
    }
    left_ .Call(SCI_GOTOLINE, static_cast<uptr_t>(target));
    right_.Call(SCI_GOTOLINE, static_cast<uptr_t>(target));
    left_.SetFocus();
}

} // namespace npp
