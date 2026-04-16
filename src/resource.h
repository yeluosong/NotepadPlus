#pragma once

#define IDI_NOTEPADL                101
#define IDR_MAIN_MENU               102
#define IDR_MAIN_ACCEL              103
#define IDR_TAB_CONTEXT             104

// File
#define IDM_FILE_NEW                40001
#define IDM_FILE_OPEN               40002
#define IDM_FILE_SAVE               40003
#define IDM_FILE_SAVEAS             40004
#define IDM_FILE_SAVEALL            40005
#define IDM_FILE_CLOSE              40006
#define IDM_FILE_CLOSEALL           40007
#define IDM_FILE_PRINT              40008
#define IDM_FILE_EXIT               40009
#define IDM_FILE_RECENT_BASE        40100   // 40100..40119 reserved for MRU
#define IDM_FILE_RECENT_MAX         40119
#define IDM_FILE_RECENT_CLEAR       40120

// Edit
#define IDM_EDIT_UNDO               40201
#define IDM_EDIT_REDO               40202
#define IDM_EDIT_CUT                40203
#define IDM_EDIT_COPY               40204
#define IDM_EDIT_PASTE              40205
#define IDM_EDIT_SELECTALL          40206
#define IDM_EDIT_DELETE             40207
#define IDM_EDIT_DUP_LINE           40208
#define IDM_EDIT_DEL_LINE           40209
#define IDM_EDIT_MOVE_UP            40210
#define IDM_EDIT_MOVE_DOWN          40211
#define IDM_EDIT_SPLIT_LINES        40212
#define IDM_EDIT_JOIN_LINES         40213
#define IDM_EDIT_TRIM_TRAIL         40214
#define IDM_EDIT_UPPERCASE          40215
#define IDM_EDIT_LOWERCASE          40216
#define IDM_EDIT_SORT_ASC           40217
#define IDM_EDIT_SORT_DESC          40218
#define IDM_EDIT_INDENT             40219
#define IDM_EDIT_OUTDENT            40220
#define IDM_EDIT_COL_EDITOR         40221
#define IDM_EDIT_COLUMN_MODE        40222
#define IDM_EDIT_BINARY_MODE        40223
#define IDM_EDIT_JSON_FORMAT        40224

// Search
#define IDM_SEARCH_FIND             40401
#define IDM_SEARCH_REPLACE          40402
#define IDM_SEARCH_FINDNEXT         40403
#define IDM_SEARCH_FINDPREV         40404
#define IDM_SEARCH_GOTO             40405
#define IDM_SEARCH_BMK_TOGGLE       40410
#define IDM_SEARCH_BMK_NEXT         40411
#define IDM_SEARCH_BMK_PREV         40412
#define IDM_SEARCH_BMK_CLEAR        40413

// Encoding
#define IDM_ENC_UTF8                40501
#define IDM_ENC_UTF8_BOM            40502
#define IDM_ENC_UTF16LE_BOM         40503
#define IDM_ENC_UTF16BE_BOM         40504
#define IDM_ENC_ANSI                40505
#define IDM_ENC_CONVERT_UTF8        40511
#define IDM_ENC_CONVERT_UTF8_BOM    40512
#define IDM_ENC_CONVERT_UTF16LE     40513
#define IDM_ENC_CONVERT_UTF16BE     40514
#define IDM_ENC_CONVERT_ANSI        40515

// Format (EOL)
#define IDM_EOL_CRLF                40601
#define IDM_EOL_LF                  40602
#define IDM_EOL_CR                  40603

// Language submenu (base id; each language offset by LangType index)
#define IDM_LANG_BASE               41000
#define IDM_LANG_MAX                41200

// Tab context menu / extra close commands
#define IDM_FILE_CLOSEBUT           40010   // Close All But Active
#define IDM_FILE_CLOSETOLEFT        40011
#define IDM_FILE_CLOSETORIGHT       40012
#define IDM_TAB_COPYPATH            40013
#define IDM_TAB_OPENFOLDER          40014

// Window navigation
#define IDM_WIN_NEXT                40301
#define IDM_WIN_PREV                40302

// View / Docking panels
#define IDM_VIEW_FOLDER_WORKSPACE   40701
#define IDM_VIEW_FUNCTION_LIST      40702
#define IDM_VIEW_DOC_MAP            40703
#define IDM_VIEW_FIND_RESULTS       40704

// Search — Find in Files
#define IDM_SEARCH_FINDFILES        40420
#define IDM_VIEW_FOLDER_OPEN        40710   // open folder in workspace

// Internal (not in menu): double-click hits in find-results panel
#define IDM_INTERNAL_FINDRES_GOTO   41500

// Find in Files dialog
#define IDD_FIND_IN_FILES           200
#define IDC_FIF_DIR                 2001
#define IDC_FIF_BROWSE              2002
#define IDC_FIF_FILTERS             2003
#define IDC_FIF_WHAT                2004
#define IDC_FIF_CASE                2005
#define IDC_FIF_WORD                2006
#define IDC_FIF_REGEX               2007
#define IDC_FIF_SUBDIRS             2008
#define IDC_FIF_FINDALL             2009

// View — split / dual view
#define IDM_VIEW_TOGGLE_SPLIT       40720
#define IDM_VIEW_MOVE_TO_OTHER      40721
#define IDM_VIEW_CLONE_TO_OTHER     40722

// Tools — Compare
#define IDM_TOOL_COMPARE            40880
#define IDM_TOOL_COMPARE_CLEAR      40881

// Column Editor dialog
#define IDD_COL_EDITOR              212
#define IDC_COL_RADIO_TEXT          2120
#define IDC_COL_RADIO_NUM           2121
#define IDC_COL_TEXT                2122
#define IDC_COL_NUM_INIT            2123
#define IDC_COL_NUM_INC             2124
#define IDC_COL_NUM_PAD             2125
#define IDC_COL_BASE_DEC            2126
#define IDC_COL_BASE_HEX            2127
#define IDC_COL_BASE_OCT            2128
#define IDC_COL_BASE_BIN            2129
#define IDC_COL_LEADZERO            2130

// Settings
#define IDM_VIEW_DARKMODE           40800

// Help
#define IDM_HELP_ABOUT              40901
