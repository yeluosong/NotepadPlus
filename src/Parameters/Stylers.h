#pragma once
#include "../ScintillaComponent/ScintillaEditView.h"
#include "LangType.h"
#include "Parameters.h"
#include <windows.h>

namespace npp {

// Apply the built-in theme + Lexilla lexer for `lang` to `view`.
// Called on buffer activation / when the user changes the language.
// Reads Parameters::Theme() to choose the active palette.
void ApplyLanguage(ScintillaEditView& view, LangType lang);

// ---- Shared UI palette ----------------------------------------------------
// Used by dialogs, tab bar, and chrome to keep a consistent look.
// Chrome layered: chromeBg (menu/toolbar/tabs) > editorBg > statusBg.
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

const UiPalette& Ui(ThemeId t);
const UiPalette& Ui();  // reads Parameters::Instance().Theme()

// ---- Toolbar icon palette -------------------------------------------------
// Each theme draws icons with its own ink + accent, so the 16×16 bitmaps
// blend into the toolbar background instead of punching through.
struct IconPalette {
    COLORREF bg;       // flood fill that matches the toolbar background
    COLORREF ink;      // primary outline
    COLORREF accent;   // primary accent stroke
    COLORREF accentDk; // darker accent (edges, depth)
    COLORREF mute;     // secondary outline
    COLORREF fill;     // interior of pages etc.
    COLORREF red;      // close / error
};

const IconPalette& Icons(ThemeId t);
const IconPalette& Icons();

} // namespace npp
