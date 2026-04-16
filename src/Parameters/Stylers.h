#pragma once
#include "../ScintillaComponent/ScintillaEditView.h"
#include "LangType.h"

namespace npp {

// Apply the built-in theme + Lexilla lexer for `lang` to `view`.
// Called on buffer activation / when the user changes the language.
// Reads Parameters::DarkMode() to choose light or dark palette.
void ApplyLanguage(ScintillaEditView& view, LangType lang);

} // namespace npp
