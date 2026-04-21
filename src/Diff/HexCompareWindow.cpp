#include "HexCompareWindow.h"
#include "../Parameters/Parameters.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <algorithm>
#include <cstdio>

namespace npp {

HexCompareWindow* HexCompareWindow::s_active = nullptr;

namespace {
constexpr wchar_t kCls[] = L"NotePadLHexCompareWnd";
constexpr int kTopBarH   = 0;   // no top bar in MVP — paths shown in title
constexpr int kStatusH   = 22;
constexpr int kSplitW    = 4;
constexpr uint64_t kMaxSize = 500ULL * 1024 * 1024;   // 500 MiB per side

void RegisterOnce(HINSTANCE hInst)
{
    static bool reg = false;
    if (reg) return;
    WNDCLASSW wc{};
    wc.lpfnWndProc   = HexCompareWindow::Proc;
    wc.hInstance     = hInst;
    wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kCls;
    ::RegisterClassW(&wc);
    reg = true;
}
}  // namespace

void HexCompareWindow::Open(HWND owner, HINSTANCE hInst,
                            const std::wstring& l, const std::wstring& r)
{
    RegisterOnce(hInst);
    if (!s_active) {
        auto* self = new HexCompareWindow();
        s_active = self;
        HWND hw = ::CreateWindowExW(0, kCls, L"Hex Compare",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1200, 700,
            owner, nullptr, hInst, self);
        if (!hw) { delete self; s_active = nullptr; return; }
    }
    if (!s_active->LoadFiles(l, r)) return;
    ::ShowWindow(s_active->hwnd_, SW_SHOW);
    ::SetForegroundWindow(s_active->hwnd_);
}

bool HexCompareWindow::LoadFiles(const std::wstring& l, const std::wstring& r)
{
    leftPath_  = l;
    rightPath_ = r;

    std::wstring err;
    if (!leftMap_.Open(l, &err)) {
        ::MessageBoxW(hwnd_, (L"Left: " + err).c_str(), L"Hex Compare", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!rightMap_.Open(r, &err)) {
        ::MessageBoxW(hwnd_, (L"Right: " + err).c_str(), L"Hex Compare", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (leftMap_.Size() > kMaxSize || rightMap_.Size() > kMaxSize) {
        ::MessageBoxW(hwnd_, L"File exceeds 500 MB limit.", L"Hex Compare", MB_OK | MB_ICONWARNING);
        leftMap_.Close(); rightMap_.Close();
        return false;
    }

    left_ .SetBuffer(leftMap_.Data(),  leftMap_.Size());
    left_ .SetPeer  (rightMap_.Data(), rightMap_.Size());
    right_.SetBuffer(rightMap_.Data(), rightMap_.Size());
    right_.SetPeer  (leftMap_.Data(),  leftMap_.Size());

    std::wstring cap = L"Hex Compare — ";
    cap += ::PathFindFileNameW(l.c_str());
    cap += L"  ↔  ";
    cap += ::PathFindFileNameW(r.c_str());
    ::SetWindowTextW(hwnd_, cap.c_str());

    UpdateStatus();
    Layout();
    left_ .Invalidate();
    right_.Invalidate();
    return true;
}

void HexCompareWindow::UpdateStatus()
{
    if (!statusBar_) return;
    uint64_t ls = leftMap_.Size(), rs = rightMap_.Size();
    uint64_t diffs = 0;
    uint64_t commonLen = std::min(ls, rs);
    const uint8_t* a = leftMap_.Data();
    const uint8_t* b = rightMap_.Data();
    if (a && b) {
        // Scan in chunks; good enough up to 500 MiB.
        for (uint64_t i = 0; i < commonLen; ++i) if (a[i] != b[i]) ++diffs;
    }
    uint64_t tail = (ls > rs) ? (ls - rs) : (rs - ls);

    wchar_t buf[192];
    ::swprintf_s(buf,
        L"Left: %llu bytes    Right: %llu bytes    Differing: %llu    Extra: %llu",
        static_cast<unsigned long long>(ls),
        static_cast<unsigned long long>(rs),
        static_cast<unsigned long long>(diffs),
        static_cast<unsigned long long>(tail));
    ::SetWindowTextW(statusBar_, buf);
}

void HexCompareWindow::Layout()
{
    if (!hwnd_) return;
    RECT rc; ::GetClientRect(hwnd_, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int bottom = h - kStatusH;
    int paneH  = bottom - kTopBarH;
    int leftW  = (w - kSplitW) / 2;
    int rightX = leftW + kSplitW;
    int rightW = w - rightX;
    ::SetWindowPos(left_.Hwnd(),  nullptr, 0,      kTopBarH, leftW,  paneH, SWP_NOZORDER);
    ::SetWindowPos(right_.Hwnd(), nullptr, rightX, kTopBarH, rightW, paneH, SWP_NOZORDER);
}

void HexCompareWindow::MirrorScroll(int src, int64_t topRow)
{
    if (inMirror_) return;
    inMirror_ = true;
    (src == 0 ? right_ : left_).SetTopRow(topRow);
    inMirror_ = false;
}

LRESULT CALLBACK HexCompareWindow::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    HexCompareWindow* self = nullptr;
    if (m == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(l);
        self = static_cast<HexCompareWindow*>(cs->lpCreateParams);
        self->hwnd_ = h;
        ::SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<HexCompareWindow*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }
    if (!self) return ::DefWindowProcW(h, m, w, l);

    HINSTANCE hInst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(h, GWLP_HINSTANCE));

    switch (m) {
    case WM_CREATE: {
        ::InitCommonControls();
        self->statusBar_ = ::CreateWindowExW(0, STATUSCLASSNAMEW, L"",
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
            h, reinterpret_cast<HMENU>(static_cast<INT_PTR>(9001)), hInst, nullptr);
        self->left_ .Create(h, hInst, 8001);
        self->right_.Create(h, hInst, 8002);
        self->left_ .SetOnScroll([self](int64_t r){ self->MirrorScroll(0, r); });
        self->right_.SetOnScroll([self](int64_t r){ self->MirrorScroll(1, r); });
        // Apply theme to the self-drawn views.
        if (Parameters::Instance().DarkMode()) {
            COLORREF bg   = RGB(0x1E, 0x1E, 0x1E);
            COLORREF fg   = RGB(0xE0, 0xE0, 0xE0);
            COLORREF diff = RGB(0x80, 0x30, 0x30);
            COLORREF miss = RGB(0x33, 0x33, 0x33);
            COLORREF off  = RGB(0x90, 0x90, 0x90);
            for (HexCompareView* v : { &self->left_, &self->right_ }) {
                v->bg_ = bg; v->fg_ = fg; v->bgDiff_ = diff;
                v->bgMiss_ = miss; v->fgOffset_ = off;
            }
        }
        return 0;
    }
    case WM_SIZE:
        self->Layout();
        if (self->statusBar_) ::SendMessageW(self->statusBar_, WM_SIZE, 0, 0);
        return 0;
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

} // namespace npp
