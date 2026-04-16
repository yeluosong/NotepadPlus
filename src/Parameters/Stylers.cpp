#include "Stylers.h"
#include "Parameters.h"

#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <SciLexer.h>
#include <ILexer.h>
#include <Lexilla.h>

#include <cstring>

namespace npp {

namespace {

// ---- Light theme palette ---------------------------------------------------
namespace light {
    constexpr COLORREF kFg        = RGB(0x00,0x00,0x00);
    constexpr COLORREF kBg        = RGB(0xFF,0xFF,0xFF);
    constexpr COLORREF kComment   = RGB(0x00,0x80,0x00);
    constexpr COLORREF kNumber    = RGB(0xFF,0x80,0x00);
    constexpr COLORREF kString    = RGB(0x80,0x80,0x80);
    constexpr COLORREF kCharacter = RGB(0x80,0x80,0x80);
    constexpr COLORREF kKeyword   = RGB(0x00,0x00,0xFF);
    constexpr COLORREF kKeyword2  = RGB(0x80,0x00,0x80);
    constexpr COLORREF kPreproc   = RGB(0x80,0x40,0x20);
    constexpr COLORREF kOperator  = RGB(0x00,0x00,0x80);
    constexpr COLORREF kIdentifier= RGB(0x00,0x00,0x00);
    constexpr COLORREF kTagName   = RGB(0x80,0x00,0x00);
    constexpr COLORREF kAttrName  = RGB(0xFF,0x00,0x00);
    constexpr COLORREF kCaretLine = RGB(0xF0,0xF0,0xF0);
    constexpr COLORREF kMargin    = RGB(0xF0,0xF0,0xF0);
    constexpr COLORREF kMarginFg  = RGB(0x90,0x90,0x90);
}

// ---- Dark theme palette (One Dark inspired) --------------------------------
namespace dark {
    constexpr COLORREF kFg        = RGB(0xD4,0xD8,0xE0);
    constexpr COLORREF kBg        = RGB(0x28,0x2C,0x34);
    constexpr COLORREF kComment   = RGB(0x70,0x78,0x88);
    constexpr COLORREF kNumber    = RGB(0xE0,0xAA,0x76);
    constexpr COLORREF kString    = RGB(0xA8,0xD3,0x89);
    constexpr COLORREF kCharacter = RGB(0xA8,0xD3,0x89);
    constexpr COLORREF kKeyword   = RGB(0xD4,0x8C,0xE8);
    constexpr COLORREF kKeyword2  = RGB(0xF0,0x80,0x88);
    constexpr COLORREF kPreproc   = RGB(0xD4,0x8C,0xE8);
    constexpr COLORREF kOperator  = RGB(0x6C,0xCC,0xD8);
    constexpr COLORREF kIdentifier= RGB(0xD4,0xD8,0xE0);
    constexpr COLORREF kTagName   = RGB(0xF0,0x80,0x88);
    constexpr COLORREF kAttrName  = RGB(0xD1,0x9A,0x66);
    constexpr COLORREF kCaretLine = RGB(0x2C,0x31,0x3A);
    constexpr COLORREF kMargin    = RGB(0x21,0x25,0x2B);
    constexpr COLORREF kMarginFg  = RGB(0x68,0x70,0x80);
}

// Active palette — set by ApplyLanguage before use.
static COLORREF sFg, sBg, sComment, sNumber, sString, sCharacter;
static COLORREF sKeyword, sKeyword2, sPreproc, sOperator, sIdentifier;
static COLORREF sTagName, sAttrName, sCaretLine, sMargin, sMarginFg;

static void SelectPalette(bool isDark) {
    if (isDark) {
        sFg = dark::kFg; sBg = dark::kBg; sComment = dark::kComment;
        sNumber = dark::kNumber; sString = dark::kString; sCharacter = dark::kCharacter;
        sKeyword = dark::kKeyword; sKeyword2 = dark::kKeyword2; sPreproc = dark::kPreproc;
        sOperator = dark::kOperator; sIdentifier = dark::kIdentifier;
        sTagName = dark::kTagName; sAttrName = dark::kAttrName;
        sCaretLine = dark::kCaretLine; sMargin = dark::kMargin; sMarginFg = dark::kMarginFg;
    } else {
        sFg = light::kFg; sBg = light::kBg; sComment = light::kComment;
        sNumber = light::kNumber; sString = light::kString; sCharacter = light::kCharacter;
        sKeyword = light::kKeyword; sKeyword2 = light::kKeyword2; sPreproc = light::kPreproc;
        sOperator = light::kOperator; sIdentifier = light::kIdentifier;
        sTagName = light::kTagName; sAttrName = light::kAttrName;
        sCaretLine = light::kCaretLine; sMargin = light::kMargin; sMarginFg = light::kMarginFg;
    }
}

void SetStyle(ScintillaEditView& v, int style, COLORREF fg, COLORREF bg)
{
    v.Call(SCI_STYLESETFORE, static_cast<uptr_t>(style), static_cast<sptr_t>(fg));
    v.Call(SCI_STYLESETBACK, static_cast<uptr_t>(style), static_cast<sptr_t>(bg));
}
void SetStyle(ScintillaEditView& v, int style, COLORREF fg)
{
    SetStyle(v, style, fg, sBg);
}

void ResetStyles(ScintillaEditView& v)
{
    v.Call(SCI_CLEARDOCUMENTSTYLE);
    v.Call(SCI_STYLESETFONT, STYLE_DEFAULT,
        reinterpret_cast<sptr_t>("Cascadia Mono"));
    v.Call(SCI_STYLESETSIZE, STYLE_DEFAULT, 11);
    SetStyle(v, STYLE_DEFAULT, sFg, sBg);
    v.Call(SCI_STYLECLEARALL);

    // Caret line highlight
    v.Call(SCI_SETCARETLINEBACK, static_cast<uptr_t>(sCaretLine));
    v.Call(SCI_SETCARETLINEVISIBLE, 1);

    // Caret color
    v.Call(SCI_SETCARETFORE, static_cast<uptr_t>(sFg));

    // Line number margin
    v.Call(SCI_STYLESETFORE, STYLE_LINENUMBER, static_cast<sptr_t>(sMarginFg));
    v.Call(SCI_STYLESETBACK, STYLE_LINENUMBER, static_cast<sptr_t>(sMargin));

    // Selection colors
    bool isDark = Parameters::Instance().DarkMode();
    if (isDark) {
        v.Call(SCI_SETSELBACK, 1, static_cast<sptr_t>(RGB(0x3E,0x44,0x51)));
        v.Call(SCI_SETSELFORE, 0, 0);
    } else {
        v.Call(SCI_SETSELBACK, 1, static_cast<sptr_t>(RGB(0xAD,0xD6,0xFF)));
        v.Call(SCI_SETSELFORE, 0, 0);
    }

    // Fold margin
    v.Call(SCI_SETFOLDMARGINCOLOUR, 1, static_cast<sptr_t>(sBg));
    v.Call(SCI_SETFOLDMARGINHICOLOUR, 1, static_cast<sptr_t>(sBg));
}

void ApplyKeywords(ScintillaEditView& v, int keywordSet, const char* words)
{
    v.Call(SCI_SETKEYWORDS, static_cast<uptr_t>(keywordSet),
        reinterpret_cast<sptr_t>(words));
}

// --- per-language style hook tables -----------------------------------------

void StyleCpp(ScintillaEditView& v)
{
    SetStyle(v, SCE_C_COMMENT,       sComment);
    SetStyle(v, SCE_C_COMMENTLINE,   sComment);
    SetStyle(v, SCE_C_COMMENTDOC,    sComment);
    SetStyle(v, SCE_C_COMMENTLINEDOC,sComment);
    SetStyle(v, SCE_C_NUMBER,        sNumber);
    SetStyle(v, SCE_C_WORD,          sKeyword);
    SetStyle(v, SCE_C_WORD2,         sKeyword2);
    SetStyle(v, SCE_C_STRING,        sString);
    SetStyle(v, SCE_C_CHARACTER,     sCharacter);
    SetStyle(v, SCE_C_OPERATOR,      sOperator);
    SetStyle(v, SCE_C_IDENTIFIER,    sIdentifier);
    SetStyle(v, SCE_C_PREPROCESSOR,  sPreproc);

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
    SetStyle(v, SCE_P_COMMENTLINE,  sComment);
    SetStyle(v, SCE_P_COMMENTBLOCK, sComment);
    SetStyle(v, SCE_P_NUMBER,       sNumber);
    SetStyle(v, SCE_P_WORD,         sKeyword);
    SetStyle(v, SCE_P_STRING,       sString);
    SetStyle(v, SCE_P_CHARACTER,    sCharacter);
    SetStyle(v, SCE_P_OPERATOR,     sOperator);
    SetStyle(v, SCE_P_IDENTIFIER,   sIdentifier);
    SetStyle(v, SCE_P_TRIPLE,       sString);
    SetStyle(v, SCE_P_TRIPLEDOUBLE, sString);
    SetStyle(v, SCE_P_DEFNAME,      sKeyword2);
    SetStyle(v, SCE_P_CLASSNAME,    sKeyword2);

    ApplyKeywords(v, 0,
        "False None True and as assert async await break class continue def del "
        "elif else except finally for from global if import in is lambda "
        "nonlocal not or pass raise return try while with yield match case");
}

void StyleHtml(ScintillaEditView& v)
{
    SetStyle(v, SCE_H_TAG,          sTagName);
    SetStyle(v, SCE_H_TAGUNKNOWN,   sTagName);
    SetStyle(v, SCE_H_ATTRIBUTE,    sAttrName);
    SetStyle(v, SCE_H_DOUBLESTRING, sString);
    SetStyle(v, SCE_H_SINGLESTRING, sString);
    SetStyle(v, SCE_H_COMMENT,      sComment);
    SetStyle(v, SCE_H_NUMBER,       sNumber);
    SetStyle(v, SCE_H_ENTITY,       sKeyword2);
    SetStyle(v, SCE_H_TAGEND,       sOperator);
    SetStyle(v, SCE_H_XMLSTART,     sTagName);
    SetStyle(v, SCE_H_XMLEND,       sTagName);
}

void StyleCss(ScintillaEditView& v)
{
    SetStyle(v, SCE_CSS_COMMENT,          sComment);
    SetStyle(v, SCE_CSS_TAG,              sTagName);
    SetStyle(v, SCE_CSS_CLASS,            sKeyword2);
    SetStyle(v, SCE_CSS_PSEUDOCLASS,      sKeyword2);
    SetStyle(v, SCE_CSS_IDENTIFIER,       sKeyword);
    SetStyle(v, SCE_CSS_DOUBLESTRING,     sString);
    SetStyle(v, SCE_CSS_SINGLESTRING,     sString);
    SetStyle(v, SCE_CSS_OPERATOR,         sOperator);
    SetStyle(v, SCE_CSS_VALUE,            sIdentifier);
    SetStyle(v, SCE_CSS_IMPORTANT,        sKeyword);
}

void StyleJson(ScintillaEditView& v)
{
    SetStyle(v, SCE_JSON_NUMBER,          sNumber);
    SetStyle(v, SCE_JSON_STRING,          sString);
    SetStyle(v, SCE_JSON_PROPERTYNAME,    sKeyword2);
    SetStyle(v, SCE_JSON_KEYWORD,         sKeyword);
    SetStyle(v, SCE_JSON_LINECOMMENT,     sComment);
    SetStyle(v, SCE_JSON_BLOCKCOMMENT,    sComment);
    SetStyle(v, SCE_JSON_OPERATOR,        sOperator);
}

void StyleXml(ScintillaEditView& v)
{
    StyleHtml(v);
}

void StyleBash(ScintillaEditView& v)
{
    SetStyle(v, SCE_SH_COMMENTLINE, sComment);
    SetStyle(v, SCE_SH_NUMBER,      sNumber);
    SetStyle(v, SCE_SH_WORD,        sKeyword);
    SetStyle(v, SCE_SH_STRING,      sString);
    SetStyle(v, SCE_SH_CHARACTER,   sCharacter);
    SetStyle(v, SCE_SH_OPERATOR,    sOperator);
    SetStyle(v, SCE_SH_IDENTIFIER,  sIdentifier);
    SetStyle(v, SCE_SH_SCALAR,      sKeyword2);

    ApplyKeywords(v, 0,
        "if then else elif fi case esac for while until do done in function "
        "return break continue exit local readonly export declare typeset "
        "select time eval exec set unset shift trap source");
}

void StyleMarkdown(ScintillaEditView& v)
{
    bool isDark = Parameters::Instance().DarkMode();

    // Headers H1..H6
    constexpr int hStyles[6] = {
        SCE_MARKDOWN_HEADER1, SCE_MARKDOWN_HEADER2, SCE_MARKDOWN_HEADER3,
        SCE_MARKDOWN_HEADER4, SCE_MARKDOWN_HEADER5, SCE_MARKDOWN_HEADER6,
    };
    COLORREF hColors[6];
    if (isDark) {
        hColors[0] = RGB(0xE0,0x6C,0x75); hColors[1] = RGB(0xD1,0x9A,0x66);
        hColors[2] = RGB(0xE5,0xC0,0x7B); hColors[3] = RGB(0x98,0xC3,0x79);
        hColors[4] = RGB(0x61,0xAF,0xEF); hColors[5] = RGB(0xC6,0x78,0xDD);
    } else {
        hColors[0] = RGB(0xC9,0x4A,0x1A); hColors[1] = RGB(0xC9,0x6B,0x2C);
        hColors[2] = RGB(0xB5,0x6E,0x2E); hColors[3] = RGB(0x8E,0x5E,0x2E);
        hColors[4] = RGB(0x6E,0x4E,0x2E); hColors[5] = RGB(0x55,0x40,0x28);
    }
    for (int i = 0; i < 6; ++i) {
        SetStyle(v, hStyles[i], hColors[i]);
        v.Call(SCI_STYLESETBOLD, hStyles[i], 1);
        v.Call(SCI_STYLESETSIZE, hStyles[i], 14 - i);
    }

    // Emphasis
    COLORREF emClr = isDark ? RGB(0xBB,0xC2,0xCF) : RGB(0x20,0x20,0x20);
    SetStyle(v, SCE_MARKDOWN_EM1,           emClr);
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_EM1, 1);
    SetStyle(v, SCE_MARKDOWN_EM2,           emClr);
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_EM2, 1);
    SetStyle(v, SCE_MARKDOWN_STRONG1,       emClr);
    v.Call(SCI_STYLESETBOLD,   SCE_MARKDOWN_STRONG1, 1);
    SetStyle(v, SCE_MARKDOWN_STRONG2,       emClr);
    v.Call(SCI_STYLESETBOLD,   SCE_MARKDOWN_STRONG2, 1);

    // Code (inline + block)
    COLORREF codeFg = isDark ? RGB(0xD1,0x9A,0x66) : RGB(0x80,0x40,0x20);
    COLORREF codeBg = isDark ? RGB(0x2E,0x33,0x3D) : RGB(0xF6,0xEE,0xDC);
    SetStyle(v, SCE_MARKDOWN_CODE,          codeFg, codeBg);
    SetStyle(v, SCE_MARKDOWN_CODE2,         codeFg, codeBg);
    SetStyle(v, SCE_MARKDOWN_CODEBK,        codeFg, codeBg);

    // Lists, blockquote, rules, links
    SetStyle(v, SCE_MARKDOWN_PRECHAR,       sKeyword2);
    SetStyle(v, SCE_MARKDOWN_ULIST_ITEM,    sKeyword);
    SetStyle(v, SCE_MARKDOWN_OLIST_ITEM,    sKeyword);
    COLORREF bqClr = isDark ? RGB(0x5C,0x63,0x70) : RGB(0x60,0x60,0x60);
    SetStyle(v, SCE_MARKDOWN_BLOCKQUOTE,    bqClr);
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_BLOCKQUOTE, 1);
    COLORREF soClr = isDark ? RGB(0x5C,0x63,0x70) : RGB(0x80,0x80,0x80);
    SetStyle(v, SCE_MARKDOWN_STRIKEOUT,     soClr);
    SetStyle(v, SCE_MARKDOWN_HRULE,         sKeyword2);
    COLORREF linkClr = isDark ? RGB(0x61,0xAF,0xEF) : RGB(0x00,0x66,0xCC);
    SetStyle(v, SCE_MARKDOWN_LINK,          linkClr);
    v.Call(SCI_STYLESETUNDERLINE, SCE_MARKDOWN_LINK, 1);
}

void StyleGeneric(ScintillaEditView& /*v*/) { /* no-op */ }

// ---------------------------------------------------------------------------

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
    SelectPalette(Parameters::Instance().DarkMode());
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
