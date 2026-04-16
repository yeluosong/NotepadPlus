# NotePad-L

A lightweight text editor for Windows, built with Win32 + Scintilla + Lexilla.

> **Declaration**
> This entire project — including all source code, CMake build system, resource files,
> and documentation — was created by **Claude Code (Anthropic Claude Opus 4.6)**.
> Human involvement was limited to providing requirements and reviewing output.

Status: **M1 skeleton** — main frame, single Scintilla view, new/open/save/save-as/close,
recent files, status bar, accelerators, UTF-8 + CRLF defaults.

## Build (Visual Studio 2022)

### 1. Fetch third-party sources

Scintilla and Lexilla are consumed as git submodules:

```cmd
git submodule update --init --recursive
```

(Or download the official tarballs from https://www.scintilla.org and unpack
them into `scintilla/` and `lexilla/` — CMake only checks for
`include/Scintilla.h` / `include/Lexilla.h`.)

### 2. Configure + build (x64 Release)

From a **x64 Native Tools Command Prompt for VS 2022**:

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build\bin\Release\notepad-l.exe`.

Or open `build\NotePad-L.sln` in Visual Studio and hit F5.

### 3. Icon

`resources/icons/app.ico` is referenced by `NotePad-L.rc`. Drop any .ico there
before building (any 32x32 ico works for M1).

## Layout

```
src/
  WinMain.cpp                        entry point
  Notepad_plus/                      app core + main frame
  ScintillaComponent/                Scintilla wrapper
  MISC/Common/                       file I/O, string helpers
  Parameters/                        recent files, config
resources/
  NotePad-L.rc, resource.h, manifest, icons/
cmake/
  Scintilla.cmake, Lexilla.cmake     static-lib builders for the submodules
```

## Roadmap

M1 skeleton  ·  M2 tabs + buffer model  ·  M3 syntax + find  ·
M4 docking + panels  ·  M5 dual view + column/multi-caret/UDL  ·
M6 macros + compare + run  ·  M7 sessions + prefs + dark mode + FTP  ·
M8 installer.

## License

This project is licensed under the [MIT License](LICENSE).

Third-party components (Scintilla, Lexilla) are used under their own permissive
licenses — see [THIRD-PARTY-NOTICES](THIRD-PARTY-NOTICES) for details.
