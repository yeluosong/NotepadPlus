#include "FolderCompareView.h"
#include <windowsx.h>
#include <algorithm>
#include <cstdio>
#include <cwchar>

namespace npp {

namespace {
constexpr wchar_t kCls[] = L"NotePadLFolderCompareView";
constexpr int kHeaderH    = 22;
constexpr int kPadX       = 6;
constexpr int kIndentPx   = 16;
constexpr int kGutterW    = 28;
constexpr int kSizeColW   = 90;
constexpr int kDateColW   = 130;
constexpr int kTriangleW  = 14;   // hit area for expand/collapse

const wchar_t* GutterGlyph(FolderDiffStatus s)
{
    switch (s) {
    case FolderDiffStatus::Same:      return L"=";
    case FolderDiffStatus::Different: return L"\u2260";  // ≠
    case FolderDiffStatus::LeftOnly:  return L"\u25C0";  // ◀
    case FolderDiffStatus::RightOnly: return L"\u25B6";  // ▶
    case FolderDiffStatus::DirOnly:   return L"=";
    }
    return L"";
}

void FormatSize(uint64_t n, wchar_t* out, size_t cap)
{
    wchar_t raw[32];
    ::swprintf_s(raw, L"%llu", static_cast<unsigned long long>(n));
    int len = static_cast<int>(::wcslen(raw));
    int outLen = len + (len - 1) / 3;
    if (outLen < 0 || static_cast<size_t>(outLen) >= cap) {
        ::wcsncpy_s(out, cap, raw, _TRUNCATE);
        return;
    }
    int di = len - 1;
    int oi = outLen - 1;
    int run = 0;
    out[outLen] = 0;
    while (di >= 0) {
        out[oi--] = raw[di--];
        if (++run == 3 && di >= 0) { out[oi--] = L','; run = 0; }
    }
}

void FormatMtime(const FILETIME& ft, wchar_t* out, size_t cap)
{
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) { out[0] = 0; return; }
    FILETIME local;
    SYSTEMTIME st;
    if (!::FileTimeToLocalFileTime(&ft, &local) || !::FileTimeToSystemTime(&local, &st)) {
        out[0] = 0;
        return;
    }
    ::swprintf_s(out, cap, L"%04u/%u/%u %02u:%02u",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
}

// Last segment of a backslash-separated relative path — the file/dir name.
const wchar_t* LeafName(const std::wstring& rel)
{
    size_t s = rel.find_last_of(L'\\');
    return s == std::wstring::npos ? rel.c_str() : rel.c_str() + s + 1;
}

void RegisterOnce(HINSTANCE hInst)
{
    static bool reg = false;
    if (reg) return;
    WNDCLASSW wc{};
    wc.lpfnWndProc   = FolderCompareView::StaticProc;
    wc.hInstance     = hInst;
    wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kCls;
    wc.style         = CS_DBLCLKS;
    ::RegisterClassW(&wc);
    reg = true;
}
}  // namespace

bool FolderCompareView::Create(HWND parent, HINSTANCE hInst, int id)
{
    RegisterOnce(hInst);
    hwnd_ = ::CreateWindowExW(WS_EX_CLIENTEDGE, kCls, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP,
        0, 0, 0, 0, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, this);
    return hwnd_ != nullptr;
}

void FolderCompareView::SetEntries(std::vector<FolderEntry> e)
{
    entries_ = std::move(e);
    expanded_.assign(entries_.size(), true);   // dirs start expanded
    RebuildVisible();
    topRow_ = 0;
    selected_ = visible_.empty() ? -1 : 0;
    UpdateScrollbar();
    Invalidate();
}

void FolderCompareView::RebuildVisible()
{
    visible_.clear();
    visible_.reserve(entries_.size());
    // Stack of (depth, expanded) for ancestor directories. If any ancestor
    // is collapsed, we hide entries beneath it.
    std::vector<std::pair<int, bool>> stk;
    bool anyAncestorCollapsed = false;
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        const FolderEntry& e = entries_[i];
        // Pop ancestors that are no longer in scope.
        while (!stk.empty() && stk.back().first >= e.depth) {
            stk.pop_back();
        }
        anyAncestorCollapsed = false;
        for (auto& p : stk) if (!p.second) { anyAncestorCollapsed = true; break; }

        if (!anyAncestorCollapsed) visible_.push_back(i);

        if (e.isDir) {
            stk.push_back({ e.depth, expanded_[i] });
        }
    }
}

void FolderCompareView::ToggleEntry(int entryIdx)
{
    if (entryIdx < 0 || entryIdx >= static_cast<int>(entries_.size())) return;
    if (!entries_[entryIdx].isDir) return;
    expanded_[entryIdx] = !expanded_[entryIdx];

    // Try to keep selection on the same logical entry; if it just became
    // hidden, fall back to the toggled directory itself.
    int prevEntry = (selected_ >= 0 && selected_ < static_cast<int>(visible_.size()))
        ? visible_[selected_] : -1;
    RebuildVisible();
    int newSel = -1;
    for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
        if (visible_[i] == prevEntry) { newSel = i; break; }
    }
    if (newSel < 0) {
        for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
            if (visible_[i] == entryIdx) { newSel = i; break; }
        }
    }
    selected_ = newSel;
    if (selected_ >= 0) EnsureVisible(selected_);
    UpdateScrollbar();
    Invalidate();
}

int FolderCompareView::TriangleX(int paneLeft, int depth) const
{
    return paneLeft + kPadX + depth * kIndentPx;
}

bool FolderCompareView::HitTriangle(int visIdx, int x) const
{
    if (visIdx < 0 || visIdx >= static_cast<int>(visible_.size())) return false;
    const FolderEntry& e = entries_[visible_[visIdx]];
    if (!e.isDir) return false;
    // The triangle sits in the left pane only; right pane is a mirror.
    RECT rc; ::GetClientRect(hwnd_, &rc);
    int W = rc.right - rc.left;
    int gutterX = (W - kGutterW) / 2;
    int leftPaneLeft = 0;
    int rightPaneLeft = gutterX + kGutterW;
    int tx1 = TriangleX(leftPaneLeft, e.depth);
    int tx2 = TriangleX(rightPaneLeft, e.depth);
    return (x >= tx1 && x < tx1 + kTriangleW) || (x >= tx2 && x < tx2 + kTriangleW);
}

void FolderCompareView::Invalidate()
{
    if (hwnd_) ::InvalidateRect(hwnd_, nullptr, FALSE);
}

void FolderCompareView::EnsureFont()
{
    if (font_) return;
    LOGFONTW lf{};
    lf.lfHeight = -13;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    ::wcscpy_s(lf.lfFaceName, L"Segoe UI");
    font_ = ::CreateFontIndirectW(&lf);

    HDC dc = ::GetDC(hwnd_);
    HFONT old = static_cast<HFONT>(::SelectObject(dc, font_));
    TEXTMETRICW tm{};
    ::GetTextMetricsW(dc, &tm);
    rowH_  = tm.tmHeight + 4;
    charW_ = tm.tmAveCharWidth;
    ::SelectObject(dc, old);
    ::ReleaseDC(hwnd_, dc);
}

int FolderCompareView::RowsVisible() const
{
    if (rowH_ <= 0) return 0;
    RECT rc; ::GetClientRect(hwnd_, &rc);
    int bodyH = (rc.bottom - rc.top) - kHeaderH;
    return std::max<int>(1, bodyH / rowH_);
}

void FolderCompareView::UpdateScrollbar()
{
    if (!hwnd_) return;
    EnsureFont();
    int total = static_cast<int>(visible_.size());
    int page  = RowsVisible();
    SCROLLINFO si{ sizeof(si) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin  = 0;
    si.nMax  = std::max(0, total - 1);
    si.nPage = page;
    si.nPos  = topRow_;
    ::SetScrollInfo(hwnd_, SB_VERT, &si, TRUE);
}

void FolderCompareView::PaintPane(HDC dc, const RECT& pane, const FolderEntry& e,
                                  bool isLeft, COLORREF rowBg, int triangleX)
{
    bool present = isLeft ? e.leftExists : e.rightExists;

    COLORREF paneBg = present ? rowBg : bgMissing_;
    HBRUSH br = ::CreateSolidBrush(paneBg);
    ::FillRect(dc, &pane, br);
    ::DeleteObject(br);

    if (!present) return;

    int dateRight = pane.right - kPadX;
    int dateLeft  = dateRight - kDateColW;
    int sizeRight = dateLeft - 8;
    int sizeLeft  = sizeRight - kSizeColW;
    int nameRight = sizeLeft - 6;
    int nameLeft  = triangleX + kTriangleW;     // name starts after the triangle
    if (nameLeft > pane.right - 20) nameLeft = pane.right - 20;

    ::SetTextColor(dc, e.status == FolderDiffStatus::Same ? fgMuted_ : fg_);

    // Name (the triangle is drawn separately by Paint()).
    std::wstring name;
    name.reserve(64);
    name += L" ";
    name += LeafName(e.relPath);
    RECT nr{ nameLeft, pane.top, nameRight, pane.bottom };
    ::DrawTextW(dc, name.c_str(), -1, &nr,
        DT_VCENTER | DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);

    if (!e.isDir) {
        wchar_t buf[40];
        uint64_t sz = isLeft ? e.leftSize : e.rightSize;
        FormatSize(sz, buf, 40);
        RECT sr{ sizeLeft, pane.top, sizeRight, pane.bottom };
        ::DrawTextW(dc, buf, -1, &sr,
            DT_VCENTER | DT_SINGLELINE | DT_RIGHT | DT_NOPREFIX);

        FormatMtime(isLeft ? e.leftMtime : e.rightMtime, buf, 40);
        RECT dr{ dateLeft, pane.top, dateRight, pane.bottom };
        ::DrawTextW(dc, buf, -1, &dr,
            DT_VCENTER | DT_SINGLELINE | DT_RIGHT | DT_NOPREFIX);
    }
}

void FolderCompareView::Paint(HDC dc)
{
    EnsureFont();
    RECT rc; ::GetClientRect(hwnd_, &rc);
    int W = rc.right - rc.left;
    int H = rc.bottom - rc.top;

    HDC mem = ::CreateCompatibleDC(dc);
    HBITMAP bmp = ::CreateCompatibleBitmap(dc, W, H);
    HBITMAP oldBmp = static_cast<HBITMAP>(::SelectObject(mem, bmp));
    HFONT oldFont = static_cast<HFONT>(::SelectObject(mem, font_));
    ::SetBkMode(mem, TRANSPARENT);

    // Pane geometry — equal halves separated by a status gutter.
    int gutterX = (W - kGutterW) / 2;
    RECT leftPane { 0,                kHeaderH, gutterX,                  H };
    RECT gutter   { gutterX,          kHeaderH, gutterX + kGutterW,       H };
    RECT rightPane{ gutterX + kGutterW, kHeaderH, W,                      H };

    // Background.
    HBRUSH bgBr = ::CreateSolidBrush(bg_);
    ::FillRect(mem, &rc, bgBr);
    ::DeleteObject(bgBr);

    // Header row.
    RECT hdr{ 0, 0, W, kHeaderH };
    HBRUSH hbr = ::CreateSolidBrush(bgHeader_);
    ::FillRect(mem, &hdr, hbr);
    ::DeleteObject(hbr);
    ::SetTextColor(mem, fg_);

    auto drawHdr = [&](const wchar_t* t, int x, int rightEdge, UINT align) {
        RECT r{ x, 0, rightEdge, kHeaderH };
        ::DrawTextW(mem, t, -1, &r, DT_VCENTER | DT_SINGLELINE | align | DT_NOPREFIX);
    };
    int leftDateRight = leftPane.right - kPadX;
    int leftDateLeft  = leftDateRight - kDateColW;
    int leftSizeRight = leftDateLeft - 8;
    int leftSizeLeft  = leftSizeRight - kSizeColW;
    drawHdr(L"Left",     leftPane.left + kPadX,  leftSizeLeft, DT_LEFT);
    drawHdr(L"Size",     leftSizeLeft,           leftSizeRight, DT_RIGHT);
    drawHdr(L"Modified", leftDateLeft,           leftDateRight, DT_RIGHT);

    int rightDateRight = rightPane.right - kPadX;
    int rightDateLeft  = rightDateRight - kDateColW;
    int rightSizeRight = rightDateLeft - 8;
    int rightSizeLeft  = rightSizeRight - kSizeColW;
    drawHdr(L"Right",    rightPane.left + kPadX, rightSizeLeft, DT_LEFT);
    drawHdr(L"Size",     rightSizeLeft,          rightSizeRight, DT_RIGHT);
    drawHdr(L"Modified", rightDateLeft,          rightDateRight, DT_RIGHT);

    // Header bottom line + pane separators.
    HPEN pen = ::CreatePen(PS_SOLID, 1, colSep_);
    HPEN oldPen = static_cast<HPEN>(::SelectObject(mem, pen));
    ::MoveToEx(mem, 0, kHeaderH - 1, nullptr); ::LineTo(mem, W, kHeaderH - 1);
    ::MoveToEx(mem, gutterX, kHeaderH, nullptr);             ::LineTo(mem, gutterX, H);
    ::MoveToEx(mem, gutterX + kGutterW - 1, kHeaderH, nullptr); ::LineTo(mem, gutterX + kGutterW - 1, H);
    ::SelectObject(mem, oldPen);
    ::DeleteObject(pen);

    // Rows (driven by the visible-index list so collapsed children disappear).
    int first = topRow_;
    int last  = std::min<int>(static_cast<int>(visible_.size()), first + RowsVisible() + 1);
    for (int i = first; i < last; ++i) {
        int entryIdx = visible_[i];
        const FolderEntry& e = entries_[entryIdx];
        int y = kHeaderH + (i - first) * rowH_;
        bool sel = (i == selected_);

        COLORREF rowBg = sel ? bgSelect_
            : (e.status == FolderDiffStatus::Different ? bgDiff_
            :  (i % 2 ? bgAlt_ : bg_));

        RECT lp{ leftPane.left,   y, leftPane.right,   y + rowH_ };
        RECT rp{ rightPane.left,  y, rightPane.right,  y + rowH_ };
        RECT gp{ gutter.left,     y, gutter.right,     y + rowH_ };

        int leftTri  = TriangleX(leftPane.left,  e.depth);
        int rightTri = TriangleX(rightPane.left, e.depth);
        PaintPane(mem, lp, e, true,  rowBg, leftTri);
        PaintPane(mem, rp, e, false, rowBg, rightTri);

        // Triangle for directories — same glyph mirrored on both panes.
        if (e.isDir) {
            const wchar_t* glyph = expanded_[entryIdx] ? L"\u25BC" : L"\u25B6";  // ▼ / ▶
            ::SetTextColor(mem, fgMuted_);
            if (e.leftExists) {
                RECT tr{ leftTri, y, leftTri + kTriangleW, y + rowH_ };
                ::DrawTextW(mem, glyph, -1, &tr,
                    DT_VCENTER | DT_SINGLELINE | DT_CENTER | DT_NOPREFIX);
            }
            if (e.rightExists) {
                RECT tr{ rightTri, y, rightTri + kTriangleW, y + rowH_ };
                ::DrawTextW(mem, glyph, -1, &tr,
                    DT_VCENTER | DT_SINGLELINE | DT_CENTER | DT_NOPREFIX);
            }
        }

        // Gutter — status glyph on a neutral background.
        HBRUSH gb = ::CreateSolidBrush(sel ? bgSelect_ : bgHeader_);
        ::FillRect(mem, &gp, gb);
        ::DeleteObject(gb);
        COLORREF gfc = fgGutter_;
        if (e.status == FolderDiffStatus::LeftOnly)  gfc = fgDel_;
        else if (e.status == FolderDiffStatus::RightOnly) gfc = fgAdd_;
        else if (e.status == FolderDiffStatus::Different) gfc = fgDel_;
        ::SetTextColor(mem, gfc);
        ::DrawTextW(mem, GutterGlyph(e.status), -1, &gp,
            DT_VCENTER | DT_SINGLELINE | DT_CENTER | DT_NOPREFIX);
    }

    ::BitBlt(dc, 0, 0, W, H, mem, 0, 0, SRCCOPY);
    ::SelectObject(mem, oldFont);
    ::SelectObject(mem, oldBmp);
    ::DeleteObject(bmp);
    ::DeleteDC(mem);
}

void FolderCompareView::ScrollBy(int rows)
{
    int total = static_cast<int>(visible_.size());
    int page  = RowsVisible();
    int maxTop = std::max(0, total - page);
    int n = std::clamp(topRow_ + rows, 0, maxTop);
    if (n != topRow_) {
        topRow_ = n;
        UpdateScrollbar();
        Invalidate();
    }
}

void FolderCompareView::OnVScroll(WPARAM w)
{
    SCROLLINFO si{ sizeof(si) };
    si.fMask = SIF_ALL;
    ::GetScrollInfo(hwnd_, SB_VERT, &si);
    int n = si.nPos;
    switch (LOWORD(w)) {
    case SB_LINEUP:       n -= 1; break;
    case SB_LINEDOWN:     n += 1; break;
    case SB_PAGEUP:       n -= si.nPage; break;
    case SB_PAGEDOWN:     n += si.nPage; break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:   n = si.nTrackPos; break;
    case SB_TOP:          n = si.nMin; break;
    case SB_BOTTOM:       n = si.nMax; break;
    }
    int total = static_cast<int>(visible_.size());
    int page  = RowsVisible();
    n = std::clamp(n, 0, std::max(0, total - page));
    if (n != topRow_) {
        topRow_ = n;
        UpdateScrollbar();
        Invalidate();
    }
}

int FolderCompareView::RowAtY(int y) const
{
    if (y < kHeaderH || rowH_ <= 0) return -1;
    int r = topRow_ + (y - kHeaderH) / rowH_;
    if (r < 0 || r >= static_cast<int>(visible_.size())) return -1;
    return r;
}

void FolderCompareView::EnsureVisible(int row)
{
    int page = RowsVisible();
    if (row < topRow_) topRow_ = row;
    else if (row >= topRow_ + page) topRow_ = row - page + 1;
    if (topRow_ < 0) topRow_ = 0;
    UpdateScrollbar();
}

LRESULT CALLBACK FolderCompareView::StaticProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    FolderCompareView* self = nullptr;
    if (m == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(l);
        self = static_cast<FolderCompareView*>(cs->lpCreateParams);
        self->hwnd_ = h;
        ::SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<FolderCompareView*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    }
    if (!self) return ::DefWindowProcW(h, m, w, l);
    return self->Proc(h, m, w, l);
}

LRESULT FolderCompareView::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = ::BeginPaint(h, &ps);
        Paint(dc);
        ::EndPaint(h, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: UpdateScrollbar(); return 0;
    case WM_VSCROLL: OnVScroll(w); return 0;
    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(w);
        ScrollBy(-delta / 40);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        ::SetFocus(h);
        int r = RowAtY(GET_Y_LPARAM(l));
        int x = GET_X_LPARAM(l);
        if (r >= 0 && HitTriangle(r, x)) {
            // Click on the triangle toggles expand/collapse without
            // moving selection elsewhere.
            ToggleEntry(visible_[r]);
            return 0;
        }
        if (r != selected_) { selected_ = r; Invalidate(); }
        return 0;
    }
    case WM_LBUTTONDBLCLK: {
        int r = RowAtY(GET_Y_LPARAM(l));
        if (r < 0) return 0;
        selected_ = r;
        const FolderEntry& e = entries_[visible_[r]];
        if (e.isDir) {
            ToggleEntry(visible_[r]);
        } else if (onActivate_) {
            onActivate_(r);
        }
        Invalidate();
        return 0;
    }
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;
    case WM_KEYDOWN: {
        int total = static_cast<int>(visible_.size());
        if (total == 0) return 0;
        int s = selected_ < 0 ? 0 : selected_;
        int page = RowsVisible();
        switch (w) {
        case VK_UP:    s = std::max(0, s - 1); break;
        case VK_DOWN:  s = std::min(total - 1, s + 1); break;
        case VK_PRIOR: s = std::max(0, s - page); break;
        case VK_NEXT:  s = std::min(total - 1, s + page); break;
        case VK_HOME:  s = 0; break;
        case VK_END:   s = total - 1; break;
        case VK_LEFT:
            // BC-style: collapse the selected dir, or jump to its parent.
            if (selected_ >= 0) {
                int eIdx = visible_[selected_];
                const FolderEntry& e = entries_[eIdx];
                if (e.isDir && expanded_[eIdx]) {
                    ToggleEntry(eIdx);
                    return 0;
                }
            }
            return 0;
        case VK_RIGHT:
            if (selected_ >= 0) {
                int eIdx = visible_[selected_];
                const FolderEntry& e = entries_[eIdx];
                if (e.isDir && !expanded_[eIdx]) {
                    ToggleEntry(eIdx);
                    return 0;
                }
            }
            return 0;
        case VK_RETURN:
            if (selected_ >= 0) {
                const FolderEntry& e = entries_[visible_[selected_]];
                if (e.isDir) ToggleEntry(visible_[selected_]);
                else if (onActivate_) onActivate_(selected_);
            }
            return 0;
        default: return 0;
        }
        selected_ = s;
        EnsureVisible(s);
        Invalidate();
        return 0;
    }
    case WM_DESTROY:
        if (font_) { ::DeleteObject(font_); font_ = nullptr; }
        return 0;
    }
    return ::DefWindowProcW(h, m, w, l);
}

} // namespace npp
