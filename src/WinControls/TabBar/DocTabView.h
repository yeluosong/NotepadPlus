#pragma once
#include "../../ScintillaComponent/Buffer.h"
#include <windows.h>
#include <commctrl.h>
#include <functional>
#include <vector>

namespace npp {

// Thin wrapper around a WC_TABCONTROL that stores a BufferID per tab and draws
// dirty-markers / close buttons owner-draw style. The dispatcher in
// Notepad_plus_Window owns all activation / close policy; this class only
// raises callbacks.
class DocTabView
{
public:
    using OnActivate = std::function<void(BufferID)>;
    using OnClose    = std::function<void(BufferID)>;
    using OnReorder  = std::function<void(BufferID fromId, int toIndex)>;
    using OnContext  = std::function<void(BufferID, POINT screen)>;
    using OnDropOut  = std::function<void(BufferID, POINT screen)>;

    bool Create(HWND parent, HINSTANCE hInst, int ctrlId);

    HWND Hwnd() const { return hwnd_; }

    // Sub-classed WndProc lives in the .cpp. Parent forwards WM_NOTIFY /
    // WM_DRAWITEM / WM_MEASUREITEM messages here so owner-draw works.
    void HandleNotify(NMHDR* hdr);
    BOOL HandleDrawItem(DRAWITEMSTRUCT* dis);
    BOOL HandleMeasureItem(MEASUREITEMSTRUCT* mis);
    void OnParentWmCommand(UINT id); // context-menu items fire here

    void AddTab(BufferID id, const std::wstring& label, bool dirty, bool activate);
    void RemoveTab(BufferID id);
    void SetLabel(BufferID id, const std::wstring& label, bool dirty);
    void Activate(BufferID id);
    void Resize(const RECT& rc);

    int        TabCount() const;
    BufferID   BufferAt(int index) const;
    int        IndexOf(BufferID id) const;
    BufferID   ActiveBuffer() const;

    void SetOnActivate(OnActivate cb) { onActivate_ = std::move(cb); }
    void SetOnClose(OnClose cb)       { onClose_    = std::move(cb); }
    void SetOnReorder(OnReorder cb)   { onReorder_  = std::move(cb); }
    void SetOnContext(OnContext cb)   { onContext_  = std::move(cb); }
    void SetOnDropOut(OnDropOut cb)   { onDropOut_  = std::move(cb); }

    // Move tab at index `from` to index `to`, preserving label + active state.
    void MoveTab(int from, int to);

private:
    static LRESULT CALLBACK TabSubclass(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    LRESULT TabProc(HWND, UINT, WPARAM, LPARAM);

    int  HitTest(POINT clientPt, bool* overCloseBtn) const;
    RECT CloseButtonRect(const RECT& tabRc) const;

    HWND hwnd_ = nullptr;
    int  ctrlId_ = 0;

    OnActivate onActivate_;
    OnClose    onClose_;
    OnReorder  onReorder_;
    OnContext  onContext_;
    OnDropOut  onDropOut_;

    // drag state
    bool dragging_     = false;
    int  dragFromIndex_= -1;
    POINT dragStart_{};

    // hover state (for showing close button only over hovered tab)
    int  hoverIndex_       = -1;
    bool hoverOverClose_   = false;
    bool hoverTracking_    = false;
};

} // namespace npp
