#include "DocTabView.h"
#include "../../Parameters/Parameters.h"
#include "../../Parameters/Stylers.h"
#include <commctrl.h>
#include <windowsx.h>
#include <cstdlib>

namespace npp {

namespace {
    constexpr UINT_PTR kSubclassId = 0xCA11AB1E;
    constexpr int kCloseBtnSize = 14;

    struct TabItemData { BufferID id; bool dirty; };
}

bool DocTabView::Create(HWND parent, HINSTANCE hInst, int ctrlId)
{
    ctrlId_ = ctrlId;
    hwnd_ = ::CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
        TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH | TCS_FOCUSNEVER | TCS_SINGLELINE | TCS_TOOLTIPS,
        0, 0, 100, 28, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ctrlId)),
        hInst, nullptr);
    if (!hwnd_) return false;

    // Modern UI font.
    LOGFONTW lf{};
    lf.lfHeight = -13;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    HFONT font = ::CreateFontIndirectW(&lf);
    if (!font) font = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    ::SendMessageW(hwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    // Fixed-size item height — owner-draw computes width itself.
    TabCtrl_SetItemSize(hwnd_, 180, 30);
    TabCtrl_SetPadding(hwnd_, 14, 4);

    ::SetWindowSubclass(hwnd_, &DocTabView::TabSubclass,
        kSubclassId, reinterpret_cast<DWORD_PTR>(this));
    return true;
}

int DocTabView::TabCount() const
{
    return TabCtrl_GetItemCount(hwnd_);
}

BufferID DocTabView::BufferAt(int index) const
{
    TCITEMW it{}; it.mask = TCIF_PARAM;
    if (!TabCtrl_GetItem(hwnd_, index, &it)) return kInvalidBufferID;
    auto* data = reinterpret_cast<TabItemData*>(it.lParam);
    return data ? data->id : kInvalidBufferID;
}

int DocTabView::IndexOf(BufferID id) const
{
    const int n = TabCount();
    for (int i = 0; i < n; ++i) {
        if (BufferAt(i) == id) return i;
    }
    return -1;
}

BufferID DocTabView::ActiveBuffer() const
{
    const int sel = TabCtrl_GetCurSel(hwnd_);
    return (sel < 0) ? kInvalidBufferID : BufferAt(sel);
}

void DocTabView::AddTab(BufferID id, const std::wstring& label, bool dirty, bool activate)
{
    auto* data = new TabItemData{ id, dirty };
    TCITEMW it{};
    it.mask    = TCIF_TEXT | TCIF_PARAM;
    it.pszText = const_cast<wchar_t*>(label.c_str());
    it.lParam  = reinterpret_cast<LPARAM>(data);
    const int idx = TabCount();
    TabCtrl_InsertItem(hwnd_, idx, &it);
    if (activate) {
        TabCtrl_SetCurSel(hwnd_, idx);
        if (onActivate_) onActivate_(id);
    }
    ::InvalidateRect(hwnd_, nullptr, TRUE);
}

void DocTabView::RemoveTab(BufferID id)
{
    const int idx = IndexOf(id);
    if (idx < 0) return;
    TCITEMW it{}; it.mask = TCIF_PARAM;
    if (TabCtrl_GetItem(hwnd_, idx, &it)) {
        delete reinterpret_cast<TabItemData*>(it.lParam);
    }
    TabCtrl_DeleteItem(hwnd_, idx);
    const int n = TabCount();
    if (n > 0) {
        int newSel = (idx < n) ? idx : n - 1;
        TabCtrl_SetCurSel(hwnd_, newSel);
        if (onActivate_) onActivate_(BufferAt(newSel));
    }
    ::InvalidateRect(hwnd_, nullptr, TRUE);
}

void DocTabView::SetLabel(BufferID id, const std::wstring& label, bool dirty)
{
    const int idx = IndexOf(id);
    if (idx < 0) return;
    TCITEMW it{}; it.mask = TCIF_PARAM;
    TabCtrl_GetItem(hwnd_, idx, &it);
    auto* data = reinterpret_cast<TabItemData*>(it.lParam);
    if (data) data->dirty = dirty;

    TCITEMW it2{};
    it2.mask    = TCIF_TEXT;
    it2.pszText = const_cast<wchar_t*>(label.c_str());
    TabCtrl_SetItem(hwnd_, idx, &it2);
    ::InvalidateRect(hwnd_, nullptr, TRUE);
}

void DocTabView::Activate(BufferID id)
{
    const int idx = IndexOf(id);
    if (idx < 0) return;
    TabCtrl_SetCurSel(hwnd_, idx);
    if (onActivate_) onActivate_(id);
    ::InvalidateRect(hwnd_, nullptr, TRUE);
}

void DocTabView::Resize(const RECT& rc)
{
    ::MoveWindow(hwnd_, rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

RECT DocTabView::CloseButtonRect(const RECT& tabRc) const
{
    RECT r;
    r.right  = tabRc.right  - 6;
    r.left   = r.right      - kCloseBtnSize;
    r.top    = tabRc.top    + (tabRc.bottom - tabRc.top - kCloseBtnSize) / 2;
    r.bottom = r.top        + kCloseBtnSize;
    return r;
}

int DocTabView::HitTest(POINT clientPt, bool* overCloseBtn) const
{
    TCHITTESTINFO hti{};
    hti.pt = clientPt;
    int idx = TabCtrl_HitTest(hwnd_, &hti);
    if (overCloseBtn) {
        *overCloseBtn = false;
        if (idx >= 0) {
            RECT rc;
            TabCtrl_GetItemRect(hwnd_, idx, &rc);
            RECT xr = CloseButtonRect(rc);
            if (::PtInRect(&xr, clientPt)) *overCloseBtn = true;
        }
    }
    return idx;
}

void DocTabView::HandleNotify(NMHDR* hdr)
{
    if (!hdr || hdr->hwndFrom != hwnd_) return;
    if (hdr->code == TCN_SELCHANGE) {
        if (onActivate_) onActivate_(ActiveBuffer());
    }
}

BOOL DocTabView::HandleMeasureItem(MEASUREITEMSTRUCT* /*mis*/)
{
    return FALSE;
}

BOOL DocTabView::HandleDrawItem(DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlID != static_cast<UINT>(ctrlId_)) return FALSE;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    const int idx = static_cast<int>(dis->itemID);

    TCITEMW it{}; it.mask = TCIF_PARAM;
    TabCtrl_GetItem(hwnd_, idx, &it);
    auto* data = reinterpret_cast<TabItemData*>(it.lParam);

    wchar_t textBuf[256] = L"";
    TCITEMW it2{};
    it2.mask       = TCIF_TEXT;
    it2.pszText    = textBuf;
    it2.cchTextMax = ARRAYSIZE(textBuf);
    TabCtrl_GetItem(hwnd_, idx, &it2);

    const bool active = (dis->itemState & ODS_SELECTED) != 0;
    const bool hover  = (idx == hoverIndex_);
    const bool isDark = Parameters::Instance().DarkMode();
    const UiPalette& u = Ui(isDark);
    // Active tab = editor bg (so it visually connects to editor below).
    // Inactive = chrome bg. Hovered inactive gets a subtle lift.
    const COLORREF bg = active ? u.editorBg
                               : (hover ? u.hotBg : u.chromeBg);
    const COLORREF fg = active ? u.text : u.textMuted;

    // Background
    HBRUSH bgBrush = ::CreateSolidBrush(bg);
    ::FillRect(hdc, &rc, bgBrush);
    ::DeleteObject(bgBrush);

    // Top accent strip on active tab (2px, flush).
    if (active) {
        RECT strip = rc; strip.bottom = strip.top + 2;
        HBRUSH ab = ::CreateSolidBrush(u.accent);
        ::FillRect(hdc, &strip, ab);
        ::DeleteObject(ab);
    }

    // Bottom separator on inactive tabs — hides on active so it joins editor.
    if (!active) {
        HPEN pen = ::CreatePen(PS_SOLID, 1, u.border);
        HPEN old = reinterpret_cast<HPEN>(::SelectObject(hdc, pen));
        ::MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
        ::LineTo  (hdc, rc.right, rc.bottom - 1);
        ::SelectObject(hdc, old);
        ::DeleteObject(pen);
    }
    // Subtle vertical divider between inactive tabs (not flanking active).
    if (!active) {
        HPEN pen = ::CreatePen(PS_SOLID, 1, u.border);
        HPEN old = reinterpret_cast<HPEN>(::SelectObject(hdc, pen));
        ::MoveToEx(hdc, rc.right - 1, rc.top + 6, nullptr);
        ::LineTo  (hdc, rc.right - 1, rc.bottom - 6);
        ::SelectObject(hdc, old);
        ::DeleteObject(pen);
    }

    // Text + optional dirty red dot.
    ::SetBkMode(hdc, TRANSPARENT);
    ::SetTextColor(hdc, fg);

    RECT textRc = rc;
    textRc.left  += 12;
    textRc.right -= kCloseBtnSize + 12;

    const bool showClose = active || hover;
    if (!showClose) textRc.right += kCloseBtnSize + 4;  // reclaim space

    // Dirty dot uses accent color when active, dirty color otherwise.
    if (data && data->dirty) {
        RECT dot;
        dot.left   = textRc.left;
        dot.top    = rc.top + (rc.bottom - rc.top) / 2 - 3;
        dot.right  = dot.left + 7;
        dot.bottom = dot.top + 7;
        HBRUSH db = ::CreateSolidBrush(u.dirty);
        ::FillRect(hdc, &dot, db);
        ::DeleteObject(db);
        textRc.left += 12;
    }

    ::DrawTextW(hdc, textBuf, -1, &textRc,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    // Close button (X) — only shown on active or hovered tabs.
    if (showClose) {
        RECT xr = CloseButtonRect(rc);
        const bool closeHot = hover && hoverOverClose_;
        if (closeHot) {
            HBRUSH hb = ::CreateSolidBrush(u.dirty);
            ::FillRect(hdc, &xr, hb);
            ::DeleteObject(hb);
        }
        COLORREF xColor = closeHot ? RGB(0xFF,0xFF,0xFF) : u.textMuted;
        HPEN xpen = ::CreatePen(PS_SOLID, 1, xColor);
        HPEN oldp = reinterpret_cast<HPEN>(::SelectObject(hdc, xpen));
        ::MoveToEx(hdc, xr.left + 4,  xr.top + 4,    nullptr);
        ::LineTo  (hdc, xr.right - 3, xr.bottom - 3);
        ::MoveToEx(hdc, xr.right - 4, xr.top + 4,    nullptr);
        ::LineTo  (hdc, xr.left + 3,  xr.bottom - 3);
        ::SelectObject(hdc, oldp);
        ::DeleteObject(xpen);
    }

    return TRUE;
}

void DocTabView::OnParentWmCommand(UINT /*id*/) {}

void DocTabView::MoveTab(int from, int to)
{
    const int n = TabCount();
    if (from < 0 || from >= n || to < 0 || to >= n || from == to) return;

    wchar_t buf[256] = L"";
    TCITEMW it{};
    it.mask       = TCIF_TEXT | TCIF_PARAM;
    it.pszText    = buf;
    it.cchTextMax = ARRAYSIZE(buf);
    if (!TabCtrl_GetItem(hwnd_, from, &it)) return;

    LPARAM saved = it.lParam;
    bool wasActive = (TabCtrl_GetCurSel(hwnd_) == from);

    TabCtrl_DeleteItem(hwnd_, from);
    int dest = to;
    if (from < to) --dest;  // indices shift after delete

    TCITEMW ins{};
    ins.mask    = TCIF_TEXT | TCIF_PARAM;
    ins.pszText = buf;
    ins.lParam  = saved;
    TabCtrl_InsertItem(hwnd_, dest, &ins);

    if (wasActive) {
        TabCtrl_SetCurSel(hwnd_, dest);
    }
    ::InvalidateRect(hwnd_, nullptr, TRUE);
}

LRESULT CALLBACK DocTabView::TabSubclass(HWND h, UINT m, WPARAM w, LPARAM l,
                                        UINT_PTR /*sub*/, DWORD_PTR ref)
{
    auto* self = reinterpret_cast<DocTabView*>(ref);
    return self ? self->TabProc(h, m, w, l)
                : ::DefSubclassProc(h, m, w, l);
}

LRESULT DocTabView::TabProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_MBUTTONUP: {
        POINT pt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        const int idx = HitTest(pt, nullptr);
        if (idx >= 0 && onClose_) onClose_(BufferAt(idx));
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINT pt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        bool overX = false;
        const int idx = HitTest(pt, &overX);
        if (idx >= 0 && overX) {
            if (onClose_) onClose_(BufferAt(idx));
            return 0;
        }
        if (idx >= 0) {
            dragging_      = true;
            dragFromIndex_ = idx;
            dragStart_     = pt;
            ::SetCapture(h);
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (dragging_) {
            ::ReleaseCapture();
            POINT pt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
            RECT cr; ::GetClientRect(h, &cr);
            BufferID draggedId = BufferAt(dragFromIndex_);

            // Drop outside this tab bar's client area → forward to host so it
            // can move/clone the tab to another view.
            if (!::PtInRect(&cr, pt)) {
                POINT screen = pt; ::ClientToScreen(h, &screen);
                if (onDropOut_ && draggedId != kInvalidBufferID)
                    onDropOut_(draggedId, screen);
            } else {
                int dest = HitTest(pt, nullptr);
                if (dest < 0) dest = TabCount() - 1; // dropped past last tab
                if (dest >= 0 && dest != dragFromIndex_) {
                    MoveTab(dragFromIndex_, dest);
                    if (onReorder_) onReorder_(draggedId, dest);
                }
            }
            dragging_      = false;
            dragFromIndex_ = -1;
            ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
        }
        break;
    }
    case WM_MOUSELEAVE: {
        if (hoverIndex_ != -1 || hoverOverClose_) {
            hoverIndex_ = -1;
            hoverOverClose_ = false;
            ::InvalidateRect(h, nullptr, TRUE);
        }
        hoverTracking_ = false;
        break;
    }
    case WM_MOUSEMOVE: {
        // Track hover for close-button reveal.
        POINT mpt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        bool overClose = false;
        int  idx = HitTest(mpt, &overClose);
        if (!hoverTracking_) {
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, h, 0 };
            ::TrackMouseEvent(&tme);
            hoverTracking_ = true;
        }
        if (idx != hoverIndex_ || overClose != hoverOverClose_) {
            hoverIndex_ = idx;
            hoverOverClose_ = overClose;
            ::InvalidateRect(h, nullptr, TRUE);
        }
        if (dragging_) {
            if ((::GetKeyState(VK_LBUTTON) & 0x8000) == 0) {
                dragging_ = false;
                dragFromIndex_ = -1;
                ::ReleaseCapture();
                ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
            } else {
                POINT pt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
                int dx = pt.x - dragStart_.x;
                int dy = pt.y - dragStart_.y;
                if (dx*dx + dy*dy > 16) {  // ~4px threshold
                    RECT cr; ::GetClientRect(h, &cr);
                    if (::PtInRect(&cr, pt))
                        ::SetCursor(::LoadCursor(nullptr, IDC_SIZEWE));
                    else
                        ::SetCursor(::LoadCursor(nullptr, IDC_HAND));
                }
            }
        }
        break;
    }
    case WM_CONTEXTMENU: {
        POINT screen{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        POINT client = screen;
        ::ScreenToClient(h, &client);
        const int idx = HitTest(client, nullptr);
        if (idx >= 0 && onContext_) onContext_(BufferAt(idx), screen);
        return 0;
    }
    }
    return ::DefSubclassProc(h, m, w, l);
}

} // namespace npp
