#pragma once
#include "../ScintillaComponent/ScintillaEditView.h"
#include "LangType.h"
#include <windows.h>

namespace npp {

// Apply the built-in theme + Lexilla lexer for `lang` to `view`.
// Called on buffer activation / when the user changes the language.
// Reads Parameters::DarkMode() to choose light or dark palette.
void ApplyLanguage(ScintillaEditView& view, LangType lang);

// ---- Shared UI palette ----------------------------------------------------
// Used by dialogs, tab bar, and chrome to keep a consistent look.
// Dark mode is layered (chrome deeper, editor lighter, status bar deepest).
struct UiPalette {
    COLORREF chromeBg;    // menu bar / tab strip / toolbar
    COLORREF editorBg;    // main edit area background
    COLORREF statusBg;    // status bar / deepest layer
    COLORREF border;      // 1px dividers
    COLORREF text;        // primary text
    COLORREF textMuted;   // secondary / inactive text
    COLORREF accent;      // highlights, active markers, default buttons
    COLORREF accentDim;   // hover / pressed variant
    COLORREF dirty;       // unsaved-changes indicator
    COLORREF caretLine;   // current line background
    COLORREF selection;   // selected text background
    COLORREF hotBg;       // hover background for items / close btn
};

const UiPalette& Ui(bool dark);
const UiPalette& Ui();  // reads Parameters::Instance().DarkMode()

} // namespace npp
