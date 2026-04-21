#include "Stylers.h"
#include "Parameters.h"

#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <SciLexer.h>
#include <ILexer.h>
#include <Lexilla.h>

#include <cstring>

namespace npp {

// ---- Shared UI palette ----------------------------------------------------
namespace {
    constexpr int kThemeCount = static_cast<int>(ThemeId::Count_);

    constexpr UiPalette kUiPalettes[kThemeCount] = {
        // 0  ModernLight (现代清爽 浅色)
        { /*chromeBg*/   RGB(0xEC,0xEE,0xF2),
          /*editorBg*/   RGB(0xFA,0xFA,0xFA),
          /*statusBg*/   RGB(0xE3,0xE5,0xE9),
          /*border*/     RGB(0xD8,0xDA,0xDE),
          /*text*/       RGB(0x1E,0x1E,0x1E),
          /*textMuted*/  RGB(0x6B,0x71,0x80),
          /*accent*/     RGB(0x2A,0x6D,0xF4),
          /*accentDim*/  RGB(0x1D,0x5B,0xD6),
          /*dirty*/      RGB(0xE5,0x48,0x4D),
          /*caretLine*/  RGB(0xF0,0xF2,0xF5),
          /*selection*/  RGB(0xAD,0xD6,0xFF),
          /*hotBg*/      RGB(0xE0,0xE4,0xEC) },
        // 1  DarkPro (暗夜专业 深色)
        { RGB(0x1B,0x1E,0x23), RGB(0x28,0x2C,0x34), RGB(0x16,0x18,0x1C),
          RGB(0x10,0x12,0x16), RGB(0xD4,0xD8,0xE0), RGB(0x80,0x85,0x90),
          RGB(0x4A,0x9E,0xFF), RGB(0x2F,0x82,0xE8), RGB(0xE0,0x6C,0x75),
          RGB(0x2C,0x31,0x3A), RGB(0x3E,0x44,0x51), RGB(0x33,0x38,0x42) },
        // 2  HighContrast (简洁高对比)
        { RGB(0xF4,0xF4,0xF4), RGB(0xFF,0xFF,0xFF), RGB(0xE8,0xE8,0xE8),
          RGB(0x90,0x90,0x90), RGB(0x00,0x00,0x00), RGB(0x44,0x44,0x44),
          RGB(0x00,0x57,0xB7), RGB(0x00,0x40,0x90), RGB(0xC8,0x1D,0x25),
          RGB(0xEE,0xF3,0xFA), RGB(0x99,0xC7,0xFF), RGB(0xDC,0xE4,0xEE) },
        // 3  Mint (柔和护眼 墨绿)
        { RGB(0xD6,0xE3,0xC9), RGB(0xE4,0xEF,0xDB), RGB(0xCB,0xDB,0xB8),
          RGB(0xA8,0xBE,0x92), RGB(0x27,0x4E,0x37), RGB(0x5E,0x7B,0x5D),
          RGB(0x3B,0x8E,0x5A), RGB(0x2A,0x6E,0x42), RGB(0xB3,0x50,0x3A),
          RGB(0xDC,0xE8,0xC4), RGB(0xBE,0xD6,0x94), RGB(0xC9,0xD8,0xAE) },
        // 4  Nordic (北欧极简 淡灰冷色)
        { RGB(0xE2,0xE6,0xED), RGB(0xEC,0xEF,0xF4), RGB(0xD8,0xDE,0xE9),
          RGB(0xB5,0xBE,0xCC), RGB(0x2E,0x34,0x40), RGB(0x61,0x6E,0x7E),
          RGB(0x5E,0x81,0xAC), RGB(0x46,0x6A,0x94), RGB(0xBF,0x61,0x6A),
          RGB(0xE5,0xEA,0xF0), RGB(0xBF,0xD3,0xE8), RGB(0xD8,0xDE,0xE9) },
        // 5  DeepBlue (代码风格 深蓝科技) — chrome/editor/status share one navy
        //     surface so the frame reads as a single block, not stacked strips.
        { /*chromeBg*/  RGB(0x11,0x20,0x36),
          /*editorBg*/  RGB(0x11,0x20,0x36),
          /*statusBg*/  RGB(0x11,0x20,0x36),
          /*border*/    RGB(0x22,0x36,0x54),
          /*text*/      RGB(0xCD,0xD6,0xE4),
          /*textMuted*/ RGB(0x7A,0x88,0x9E),
          /*accent*/    RGB(0x4E,0xC9,0xB0),
          /*accentDim*/ RGB(0x35,0xA6,0x92),
          /*dirty*/     RGB(0xF0,0x7B,0x8A),
          /*caretLine*/ RGB(0x18,0x2B,0x48),
          /*selection*/ RGB(0x24,0x49,0x72),
          /*hotBg*/     RGB(0x1D,0x31,0x4F) },
    };

    constexpr IconPalette kIconPalettes[kThemeCount] = {
        // 0  ModernLight: ink gray + sky-blue accent, white fill
        { /*bg*/ RGB(0xFA,0xF7,0xF5), RGB(0x9A,0xA4,0xB0), RGB(0x6E,0xB4,0xEE),
          RGB(0x4C,0x9A,0xD9), RGB(0xC8,0xCE,0xD6), RGB(0xFF,0xFF,0xFF),
          RGB(0xE0,0x8A,0x8A) },
        // 1  DarkPro: warm charcoal bg, cool ink
        { RGB(0x2B,0x25,0x21), RGB(0x9A,0xA4,0xB0), RGB(0x6E,0xB4,0xEE),
          RGB(0x4C,0x9A,0xD9), RGB(0x5C,0x63,0x70), RGB(0xE8,0xEB,0xF0),
          RGB(0xE0,0x8A,0x8A) },
        // 2  HighContrast: strong black ink on white
        { RGB(0xF4,0xF4,0xF4), RGB(0x20,0x20,0x20), RGB(0x00,0x57,0xB7),
          RGB(0x00,0x40,0x90), RGB(0x70,0x70,0x70), RGB(0xFF,0xFF,0xFF),
          RGB(0xC8,0x1D,0x25) },
        // 3  Mint: dark green ink on sage background
        { RGB(0xD6,0xE3,0xC9), RGB(0x4E,0x6A,0x4F), RGB(0x3B,0x8E,0x5A),
          RGB(0x2A,0x6E,0x42), RGB(0x95,0xA9,0x88), RGB(0xE4,0xEF,0xDB),
          RGB(0xB3,0x50,0x3A) },
        // 4  Nordic: slate ink + frost-blue accent
        { RGB(0xE2,0xE6,0xED), RGB(0x4C,0x56,0x6A), RGB(0x5E,0x81,0xAC),
          RGB(0x46,0x6A,0x94), RGB(0x9C,0xA8,0xBB), RGB(0xEC,0xEF,0xF4),
          RGB(0xBF,0x61,0x6A) },
        // 5  DeepBlue: teal-accent ink on deep blue (bg matches chromeBg).
        { RGB(0x11,0x20,0x36), RGB(0x8A,0x99,0xAE), RGB(0x4E,0xC9,0xB0),
          RGB(0x35,0xA6,0x92), RGB(0x4A,0x55,0x68), RGB(0xCD,0xD6,0xE4),
          RGB(0xF0,0x7B,0x8A) },
    };

    inline int ThemeIndex(ThemeId t) {
        int i = static_cast<int>(t);
        if (i < 0 || i >= kThemeCount) i = 0;
        return i;
    }
}

const UiPalette& Ui(ThemeId t) { return kUiPalettes[ThemeIndex(t)]; }
const UiPalette& Ui() { return Ui(Parameters::Instance().Theme()); }

const IconPalette& Icons(ThemeId t) { return kIconPalettes[ThemeIndex(t)]; }
const IconPalette& Icons() { return Icons(Parameters::Instance().Theme()); }

// ---- Per-theme syntax palette --------------------------------------------
namespace {

struct SyntaxPalette {
    COLORREF fg;         // default text
    COLORREF bg;         // editor background (must match UiPalette.editorBg)
    COLORREF comment;
    COLORREF number;
    COLORREF string;
    COLORREF character;
    COLORREF keyword;
    COLORREF keyword2;   // secondary keyword / type
    COLORREF preproc;
    COLORREF op;
    COLORREF identifier;
    COLORREF tagName;
    COLORREF attrName;
    COLORREF caretLine;
    COLORREF margin;
    COLORREF marginFg;
};

constexpr SyntaxPalette kSyntax[kThemeCount] = {
    // 0 ModernLight
    { RGB(0x00,0x00,0x00), RGB(0xFA,0xFA,0xFA),
      RGB(0x00,0x80,0x00), RGB(0xFF,0x80,0x00),
      RGB(0x80,0x80,0x80), RGB(0x80,0x80,0x80),
      RGB(0x00,0x00,0xFF), RGB(0x80,0x00,0x80),
      RGB(0x80,0x40,0x20), RGB(0x00,0x00,0x80),
      RGB(0x00,0x00,0x00), RGB(0x80,0x00,0x00),
      RGB(0xFF,0x00,0x00),
      RGB(0xF0,0xF2,0xF5), RGB(0xF2,0xF3,0xF6), RGB(0x98,0xA0,0xAE) },
    // 1 DarkPro (One Dark inspired)
    { RGB(0xD4,0xD8,0xE0), RGB(0x28,0x2C,0x34),
      RGB(0x70,0x78,0x88), RGB(0xE0,0xAA,0x76),
      RGB(0xA8,0xD3,0x89), RGB(0xA8,0xD3,0x89),
      RGB(0xD4,0x8C,0xE8), RGB(0xF0,0x80,0x88),
      RGB(0xD4,0x8C,0xE8), RGB(0x6C,0xCC,0xD8),
      RGB(0xD4,0xD8,0xE0), RGB(0xF0,0x80,0x88),
      RGB(0xD1,0x9A,0x66),
      RGB(0x2C,0x31,0x3A), RGB(0x1E,0x21,0x27), RGB(0x68,0x70,0x80) },
    // 2 HighContrast
    { RGB(0x00,0x00,0x00), RGB(0xFF,0xFF,0xFF),
      RGB(0x00,0x66,0x00), RGB(0xB0,0x45,0x00),
      RGB(0x85,0x15,0x85), RGB(0x85,0x15,0x85),
      RGB(0x00,0x00,0xC8), RGB(0x80,0x00,0x80),
      RGB(0x80,0x20,0x00), RGB(0x00,0x00,0x80),
      RGB(0x00,0x00,0x00), RGB(0x80,0x00,0x00),
      RGB(0xC0,0x00,0x00),
      RGB(0xEE,0xF3,0xFA), RGB(0xEE,0xEE,0xEE), RGB(0x66,0x66,0x66) },
    // 3 Mint
    { RGB(0x27,0x4E,0x37), RGB(0xE4,0xEF,0xDB),
      RGB(0x6F,0x8C,0x5A), RGB(0xB0,0x6A,0x2A),
      RGB(0x48,0x72,0x5A), RGB(0x48,0x72,0x5A),
      RGB(0x2B,0x63,0x90), RGB(0x75,0x3E,0x8C),
      RGB(0x8B,0x4A,0x2C), RGB(0x3F,0x67,0x52),
      RGB(0x27,0x4E,0x37), RGB(0x6C,0x35,0x76),
      RGB(0xB0,0x43,0x2E),
      RGB(0xDC,0xE8,0xC4), RGB(0xDC,0xE8,0xCA), RGB(0x8A,0xA0,0x7E) },
    // 4 Nordic
    { RGB(0x2E,0x34,0x40), RGB(0xEC,0xEF,0xF4),
      RGB(0x81,0x8F,0x9E), RGB(0xB4,0x8E,0x4D),
      RGB(0xA3,0xBE,0x8C), RGB(0xA3,0xBE,0x8C),
      RGB(0x5E,0x81,0xAC), RGB(0xB4,0x8E,0xAD),
      RGB(0xD0,0x87,0x70), RGB(0x81,0xA1,0xC1),
      RGB(0x2E,0x34,0x40), RGB(0xBF,0x61,0x6A),
      RGB(0xEB,0xCB,0x8B),
      RGB(0xE5,0xEA,0xF0), RGB(0xE2,0xE6,0xED), RGB(0x8D,0x99,0xAC) },
    // 5 DeepBlue — bg + margin align with UiPalette so no seams appear.
    { RGB(0xCD,0xD6,0xE4), RGB(0x11,0x20,0x36),
      RGB(0x6A,0x93,0x55), RGB(0xE5,0xB8,0x73),
      RGB(0xCE,0x91,0x78), RGB(0xCE,0x91,0x78),
      RGB(0x56,0x9C,0xD6), RGB(0x4E,0xC9,0xB0),
      RGB(0xC5,0x86,0xC0), RGB(0xD4,0xD4,0xD4),
      RGB(0xCD,0xD6,0xE4), RGB(0x56,0x9C,0xD6),
      RGB(0x9C,0xDC,0xFE),
      RGB(0x18,0x2B,0x48), RGB(0x11,0x20,0x36), RGB(0x55,0x6B,0x88) },
};

inline const SyntaxPalette& Syntax(ThemeId t) { return kSyntax[ThemeIndex(t)]; }

// Active palette — set by ApplyLanguage before use.
static SyntaxPalette sP;

static void SelectPalette(ThemeId t) { sP = Syntax(t); }

void SetStyle(ScintillaEditView& v, int style, COLORREF fg, COLORREF bg)
{
    v.Call(SCI_STYLESETFORE, static_cast<uptr_t>(style), static_cast<sptr_t>(fg));
    v.Call(SCI_STYLESETBACK, static_cast<uptr_t>(style), static_cast<sptr_t>(bg));
}
void SetStyle(ScintillaEditView& v, int style, COLORREF fg)
{
    SetStyle(v, style, fg, sP.bg);
}

void ResetStyles(ScintillaEditView& v)
{
    v.Call(SCI_CLEARDOCUMENTSTYLE);
    v.Call(SCI_STYLESETFONT, STYLE_DEFAULT,
        reinterpret_cast<sptr_t>("Cascadia Mono"));
    v.Call(SCI_STYLESETSIZE, STYLE_DEFAULT, 11);
    SetStyle(v, STYLE_DEFAULT, sP.fg, sP.bg);
    v.Call(SCI_STYLECLEARALL);

    v.Call(SCI_SETCARETLINEBACK, static_cast<uptr_t>(sP.caretLine));
    v.Call(SCI_SETCARETLINEVISIBLE, 1);
    v.Call(SCI_SETCARETFORE, static_cast<uptr_t>(sP.fg));

    v.Call(SCI_STYLESETFORE, STYLE_LINENUMBER, static_cast<sptr_t>(sP.marginFg));
    v.Call(SCI_STYLESETBACK, STYLE_LINENUMBER, static_cast<sptr_t>(sP.margin));

    v.Call(SCI_SETCARETLINEFRAME, 0);
    v.Call(SCI_SETCARETWIDTH, 2);

    ThemeId tid = Parameters::Instance().Theme();
    const UiPalette& u = Ui(tid);
    v.Call(SCI_SETSELBACK, 1, static_cast<sptr_t>(u.selection));
    v.Call(SCI_SETSELFORE, 0, 0);

    // Bracket matching uses accent fore + underline/bold.
    v.Call(SCI_STYLESETFORE, STYLE_BRACELIGHT, static_cast<sptr_t>(u.accent));
    v.Call(SCI_STYLESETBACK, STYLE_BRACELIGHT, static_cast<sptr_t>(sP.bg));
    v.Call(SCI_STYLESETBOLD, STYLE_BRACELIGHT, 1);
    v.Call(SCI_STYLESETUNDERLINE, STYLE_BRACELIGHT, 1);
    v.Call(SCI_STYLESETFORE, STYLE_BRACEBAD, static_cast<sptr_t>(u.dirty));
    v.Call(SCI_STYLESETBOLD, STYLE_BRACEBAD, 1);

    v.Call(SCI_SETWHITESPACEFORE, 1, static_cast<sptr_t>(u.textMuted));

    v.Call(SCI_SETFOLDMARGINCOLOUR, 1, static_cast<sptr_t>(sP.bg));
    v.Call(SCI_SETFOLDMARGINHICOLOUR, 1, static_cast<sptr_t>(sP.bg));
}

void ApplyKeywords(ScintillaEditView& v, int keywordSet, const char* words)
{
    v.Call(SCI_SETKEYWORDS, static_cast<uptr_t>(keywordSet),
        reinterpret_cast<sptr_t>(words));
}

void StyleCpp(ScintillaEditView& v)
{
    SetStyle(v, SCE_C_COMMENT,       sP.comment);
    SetStyle(v, SCE_C_COMMENTLINE,   sP.comment);
    SetStyle(v, SCE_C_COMMENTDOC,    sP.comment);
    SetStyle(v, SCE_C_COMMENTLINEDOC,sP.comment);
    SetStyle(v, SCE_C_NUMBER,        sP.number);
    SetStyle(v, SCE_C_WORD,          sP.keyword);
    SetStyle(v, SCE_C_WORD2,         sP.keyword2);
    SetStyle(v, SCE_C_STRING,        sP.string);
    SetStyle(v, SCE_C_CHARACTER,     sP.character);
    SetStyle(v, SCE_C_OPERATOR,      sP.op);
    SetStyle(v, SCE_C_IDENTIFIER,    sP.identifier);
    SetStyle(v, SCE_C_PREPROCESSOR,  sP.preproc);

    ApplyKeywords(v, 0,
        "alignas alignof and and_eq asm auto bitand bitor bool break case catch "
        "char char8_t char16_t char32_t class compl concept const consteval "
        "constexpr constinit const_cast continue co_await co_return co_yield "
        "decltype default delete do double dynamic_cast else enum explicit "
        "export extern false float for friend goto if inline int long mutable "
        "namespace new noexcept not not_eq nullptr operator or or_eq private "
        "protected public register reinterpret_cast requires return short "
        "signed sizeof static static_assert static_cast struct switch template "
        "this thread_local throw true try typedef typeid typename union "
        "unsigned using virtual void volatile wchar_t while xor xor_eq");
}

void StylePython(ScintillaEditView& v)
{
    SetStyle(v, SCE_P_COMMENTLINE,  sP.comment);
    SetStyle(v, SCE_P_COMMENTBLOCK, sP.comment);
    SetStyle(v, SCE_P_NUMBER,       sP.number);
    SetStyle(v, SCE_P_WORD,         sP.keyword);
    SetStyle(v, SCE_P_STRING,       sP.string);
    SetStyle(v, SCE_P_CHARACTER,    sP.character);
    SetStyle(v, SCE_P_OPERATOR,     sP.op);
    SetStyle(v, SCE_P_IDENTIFIER,   sP.identifier);
    SetStyle(v, SCE_P_TRIPLE,       sP.string);
    SetStyle(v, SCE_P_TRIPLEDOUBLE, sP.string);
    SetStyle(v, SCE_P_DEFNAME,      sP.keyword2);
    SetStyle(v, SCE_P_CLASSNAME,    sP.keyword2);

    ApplyKeywords(v, 0,
        "False None True and as assert async await break class continue def del "
        "elif else except finally for from global if import in is lambda "
        "nonlocal not or pass raise return try while with yield match case");
}

void StyleHtml(ScintillaEditView& v)
{
    SetStyle(v, SCE_H_TAG,          sP.tagName);
    SetStyle(v, SCE_H_TAGUNKNOWN,   sP.tagName);
    SetStyle(v, SCE_H_ATTRIBUTE,    sP.attrName);
    SetStyle(v, SCE_H_DOUBLESTRING, sP.string);
    SetStyle(v, SCE_H_SINGLESTRING, sP.string);
    SetStyle(v, SCE_H_COMMENT,      sP.comment);
    SetStyle(v, SCE_H_NUMBER,       sP.number);
    SetStyle(v, SCE_H_ENTITY,       sP.keyword2);
    SetStyle(v, SCE_H_TAGEND,       sP.op);
    SetStyle(v, SCE_H_XMLSTART,     sP.tagName);
    SetStyle(v, SCE_H_XMLEND,       sP.tagName);
}

void StyleCss(ScintillaEditView& v)
{
    SetStyle(v, SCE_CSS_COMMENT,          sP.comment);
    SetStyle(v, SCE_CSS_TAG,              sP.tagName);
    SetStyle(v, SCE_CSS_CLASS,            sP.keyword2);
    SetStyle(v, SCE_CSS_PSEUDOCLASS,      sP.keyword2);
    SetStyle(v, SCE_CSS_IDENTIFIER,       sP.keyword);
    SetStyle(v, SCE_CSS_DOUBLESTRING,     sP.string);
    SetStyle(v, SCE_CSS_SINGLESTRING,     sP.string);
    SetStyle(v, SCE_CSS_OPERATOR,         sP.op);
    SetStyle(v, SCE_CSS_VALUE,            sP.identifier);
    SetStyle(v, SCE_CSS_IMPORTANT,        sP.keyword);
}

void StyleJson(ScintillaEditView& v)
{
    SetStyle(v, SCE_JSON_NUMBER,          sP.number);
    SetStyle(v, SCE_JSON_STRING,          sP.string);
    SetStyle(v, SCE_JSON_PROPERTYNAME,    sP.keyword2);
    SetStyle(v, SCE_JSON_KEYWORD,         sP.keyword);
    SetStyle(v, SCE_JSON_LINECOMMENT,     sP.comment);
    SetStyle(v, SCE_JSON_BLOCKCOMMENT,    sP.comment);
    SetStyle(v, SCE_JSON_OPERATOR,        sP.op);
}

void StyleXml(ScintillaEditView& v)
{
    StyleHtml(v);
}

void StyleBash(ScintillaEditView& v)
{
    SetStyle(v, SCE_SH_COMMENTLINE, sP.comment);
    SetStyle(v, SCE_SH_NUMBER,      sP.number);
    SetStyle(v, SCE_SH_WORD,        sP.keyword);
    SetStyle(v, SCE_SH_STRING,      sP.string);
    SetStyle(v, SCE_SH_CHARACTER,   sP.character);
    SetStyle(v, SCE_SH_OPERATOR,    sP.op);
    SetStyle(v, SCE_SH_IDENTIFIER,  sP.identifier);
    SetStyle(v, SCE_SH_SCALAR,      sP.keyword2);

    ApplyKeywords(v, 0,
        "if then else elif fi case esac for while until do done in function "
        "return break continue exit local readonly export declare typeset "
        "select time eval exec set unset shift trap source");
}

void StyleMarkdown(ScintillaEditView& v)
{
    ThemeId tid = Parameters::Instance().Theme();
    bool isDark = IsDarkFamily(tid);

    constexpr int hStyles[6] = {
        SCE_MARKDOWN_HEADER1, SCE_MARKDOWN_HEADER2, SCE_MARKDOWN_HEADER3,
        SCE_MARKDOWN_HEADER4, SCE_MARKDOWN_HEADER5, SCE_MARKDOWN_HEADER6,
    };
    // Per-theme header color ramps. Headers always stay saturated so
    // structure reads even against muted backgrounds.
    COLORREF hColors[6];
    switch (tid) {
    case ThemeId::DarkPro:
        hColors[0] = RGB(0xE0,0x6C,0x75); hColors[1] = RGB(0xD1,0x9A,0x66);
        hColors[2] = RGB(0xE5,0xC0,0x7B); hColors[3] = RGB(0x98,0xC3,0x79);
        hColors[4] = RGB(0x61,0xAF,0xEF); hColors[5] = RGB(0xC6,0x78,0xDD);
        break;
    case ThemeId::DeepBlue:
        hColors[0] = RGB(0x4E,0xC9,0xB0); hColors[1] = RGB(0x56,0x9C,0xD6);
        hColors[2] = RGB(0x9C,0xDC,0xFE); hColors[3] = RGB(0xCE,0x91,0x78);
        hColors[4] = RGB(0xC5,0x86,0xC0); hColors[5] = RGB(0xDC,0xDC,0xAA);
        break;
    case ThemeId::Mint:
        hColors[0] = RGB(0x1E,0x5E,0x3A); hColors[1] = RGB(0x35,0x72,0x48);
        hColors[2] = RGB(0x4E,0x87,0x56); hColors[3] = RGB(0x66,0x9B,0x62);
        hColors[4] = RGB(0x7B,0xAF,0x6F); hColors[5] = RGB(0x8F,0xBF,0x7C);
        break;
    case ThemeId::Nordic:
        hColors[0] = RGB(0xBF,0x61,0x6A); hColors[1] = RGB(0xD0,0x87,0x70);
        hColors[2] = RGB(0xEB,0xCB,0x8B); hColors[3] = RGB(0xA3,0xBE,0x8C);
        hColors[4] = RGB(0x5E,0x81,0xAC); hColors[5] = RGB(0xB4,0x8E,0xAD);
        break;
    case ThemeId::HighContrast:
        hColors[0] = RGB(0xA0,0x00,0x00); hColors[1] = RGB(0x80,0x2A,0x00);
        hColors[2] = RGB(0x60,0x55,0x00); hColors[3] = RGB(0x00,0x60,0x20);
        hColors[4] = RGB(0x00,0x40,0x90); hColors[5] = RGB(0x5A,0x00,0x7A);
        break;
    case ThemeId::ModernLight:
    default:
        hColors[0] = RGB(0xC9,0x4A,0x1A); hColors[1] = RGB(0xC9,0x6B,0x2C);
        hColors[2] = RGB(0xB5,0x6E,0x2E); hColors[3] = RGB(0x8E,0x5E,0x2E);
        hColors[4] = RGB(0x6E,0x4E,0x2E); hColors[5] = RGB(0x55,0x40,0x28);
        break;
    }
    for (int i = 0; i < 6; ++i) {
        SetStyle(v, hStyles[i], hColors[i]);
        v.Call(SCI_STYLESETBOLD, hStyles[i], 1);
        v.Call(SCI_STYLESETSIZE, hStyles[i], 14 - i);
    }

    COLORREF emClr = isDark ? RGB(0xBB,0xC2,0xCF) : sP.fg;
    SetStyle(v, SCE_MARKDOWN_EM1,     emClr);
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_EM1, 1);
    SetStyle(v, SCE_MARKDOWN_EM2,     emClr);
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_EM2, 1);
    SetStyle(v, SCE_MARKDOWN_STRONG1, emClr);
    v.Call(SCI_STYLESETBOLD,   SCE_MARKDOWN_STRONG1, 1);
    SetStyle(v, SCE_MARKDOWN_STRONG2, emClr);
    v.Call(SCI_STYLESETBOLD,   SCE_MARKDOWN_STRONG2, 1);

    // Code block: tinted from the theme's caret line (already on-theme).
    COLORREF codeFg = sP.preproc;
    COLORREF codeBg = sP.caretLine;
    SetStyle(v, SCE_MARKDOWN_CODE,    codeFg, codeBg);
    SetStyle(v, SCE_MARKDOWN_CODE2,   codeFg, codeBg);
    SetStyle(v, SCE_MARKDOWN_CODEBK,  codeFg, codeBg);

    SetStyle(v, SCE_MARKDOWN_PRECHAR,     sP.keyword2);
    SetStyle(v, SCE_MARKDOWN_ULIST_ITEM,  sP.keyword);
    SetStyle(v, SCE_MARKDOWN_OLIST_ITEM,  sP.keyword);
    SetStyle(v, SCE_MARKDOWN_BLOCKQUOTE,  sP.comment);
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_BLOCKQUOTE, 1);
    SetStyle(v, SCE_MARKDOWN_STRIKEOUT,   sP.comment);
    SetStyle(v, SCE_MARKDOWN_HRULE,       sP.keyword2);

    const UiPalette& u = Ui(tid);
    SetStyle(v, SCE_MARKDOWN_LINK, u.accent);
    v.Call(SCI_STYLESETUNDERLINE, SCE_MARKDOWN_LINK, 1);
}

void StyleGeneric(ScintillaEditView& /*v*/) { /* no-op */ }

void SetLexerByName(ScintillaEditView& v, const char* lexerName)
{
    if (!lexerName || !*lexerName) {
        v.Call(SCI_SETILEXER, 0, 0);
        return;
    }
    void* lexer = reinterpret_cast<void*>(CreateLexer(lexerName));
    v.Call(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
}

} // namespace

void ApplyLanguage(ScintillaEditView& v, LangType lang)
{
    SelectPalette(Parameters::Instance().Theme());
    ResetStyles(v);
    SetLexerByName(v, LangLexerName(lang));

    switch (lang) {
    case LangType::C:
    case LangType::Cpp:
    case LangType::CSharp:
    case LangType::Java:
    case LangType::ObjectiveC:
    case LangType::JavaScript:
    case LangType::TypeScript:
    case LangType::Go:
    case LangType::Swift:
    case LangType::Rust:
    case LangType::Kotlin:
    case LangType::Dart:
    case LangType::Scala:
        StyleCpp(v); break;
    case LangType::Python:
        StylePython(v); break;
    case LangType::Html:
    case LangType::PhP:
        StyleHtml(v); break;
    case LangType::Xml:
        StyleXml(v); break;
    case LangType::Css:
    case LangType::Scss:
    case LangType::Less:
        StyleCss(v); break;
    case LangType::Json:
        StyleJson(v); break;
    case LangType::Shell:
        StyleBash(v); break;
    case LangType::Markdown:
        StyleMarkdown(v); break;
    default:
        StyleGeneric(v); break;
    }

    v.Call(SCI_COLOURISE, 0, -1);
}

} // namespace npp
