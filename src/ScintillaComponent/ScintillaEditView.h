#pragma once
#include <windows.h>
#include <string>
#include <cstdint>

// Forward-declared Scintilla message type; we avoid including Scintilla.h here
// so the public surface stays small.
typedef intptr_t sptr_t;
typedef uintptr_t uptr_t;

namespace npp {

// Thin wrapper around a Scintilla window. One instance per editor view.
// M1 scope: create/destroy, load/save text (UTF-8), dirty tracking.
class ScintillaEditView
{
public:
    ScintillaEditView() = default;
    ~ScintillaEditView();

    ScintillaEditView(const ScintillaEditView&) = delete;
    ScintillaEditView& operator=(const ScintillaEditView&) = delete;

    bool Create(HWND parent, HINSTANCE hInst);
    void Destroy();   // explicit teardown; safe to call before destructor
    HWND Hwnd() const { return hwnd_; }

    // Resize the Scintilla child to fill 'rc'.
    void Resize(const RECT& rc);

    // Direct-function call path (faster than SendMessage). Initialized in Create().
    sptr_t Call(unsigned int msg, uptr_t wParam = 0, sptr_t lParam = 0) const;

    // Text I/O
    void SetText(const std::string& utf8);
    std::string GetText() const;
    bool IsDirty() const;
    void MarkSaved();
    void ClearAll();

    // Attach a Scintilla document handle (obtained from SCI_CREATEDOCUMENT).
    // Scintilla handles ref counting internally when you swap documents.
    void AttachDocument(sptr_t docHandle);

    void SetFocus();

private:
    HWND                 hwnd_     = nullptr;
    using DirectFn = sptr_t (*)(sptr_t, unsigned int, uptr_t, sptr_t);
    DirectFn             direct_   = nullptr;
    sptr_t               directPtr_= 0;

    void ApplyDefaultStyle();
};

} // namespace npp
