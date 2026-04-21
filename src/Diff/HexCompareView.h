#pragma once
#include <windows.h>
#include <cstdint>
#include <functional>

namespace npp {

// Self-drawn hex viewer. Non-owning view over a byte buffer (typically a
// memory-mapped file). One row = 16 bytes:
//     OFFSET   HH HH HH HH HH HH HH HH  HH HH HH HH HH HH HH HH  |ascii..........|
//
// For diff coloring the caller must point OtherBuffer() at the peer view's
// data — bytes that differ at the same offset get a red background. Also
// shows "missing" bytes (past one side's EOF) in grey.
class HexCompareView
{
public:
    static constexpr int kBytesPerRow = 16;

    bool Create(HWND parent, HINSTANCE hInst, int id);
    HWND Hwnd() const { return hwnd_; }

    // Non-owning; caller must keep buffer alive for the view's lifetime.
    void SetBuffer(const uint8_t* data, uint64_t size);
    void SetPeer   (const uint8_t* data, uint64_t size);
    void SetTopRow (int64_t row);
    int64_t TopRow () const { return topRow_; }
    int64_t RowCount() const;
    int  RowsVisible() const { return rowsVisible_; }

    // Invoked whenever this view's top row changes (scroll / keys / wheel).
    // Parent uses this to mirror scroll to the peer view.
    void SetOnScroll(std::function<void(int64_t)> cb) { onScroll_ = std::move(cb); }

    void Invalidate();
    void UpdateScrollbar();

    static LRESULT CALLBACK StaticProc(HWND, UINT, WPARAM, LPARAM);

    // Colors (light mode defaults; caller can override before first paint).
    COLORREF bg_       = RGB(0xFF, 0xFF, 0xFF);
    COLORREF fg_       = RGB(0x1E, 0x1E, 0x1E);
    COLORREF bgDiff_   = RGB(0xFF, 0xCC, 0xCC);
    COLORREF bgMiss_   = RGB(0xE0, 0xE0, 0xE0);
    COLORREF fgOffset_ = RGB(0x66, 0x66, 0x66);

private:
    LRESULT Proc(HWND, UINT, WPARAM, LPARAM);

    void EnsureFont();
    void Paint(HDC dc);
    void ScrollBy(int64_t rows);
    void OnVScroll(WPARAM w);
    void OnSize();

    HWND           hwnd_ = nullptr;
    HFONT          font_ = nullptr;
    int            cellW_ = 0, cellH_ = 0;

    const uint8_t* data_  = nullptr;
    uint64_t       size_  = 0;
    const uint8_t* peer_  = nullptr;
    uint64_t       peerSz_= 0;
    int64_t        topRow_ = 0;
    int            rowsVisible_ = 0;
    std::function<void(int64_t)> onScroll_;
};

} // namespace npp
