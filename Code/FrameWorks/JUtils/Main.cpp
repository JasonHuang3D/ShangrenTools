//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Main.h"

#if defined(WIN32) && defined(M_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // (WIN32) && defined (M_DEBUG)
#include <sstream>

namespace JUtils
{
bool ConfigPlatformCMD()
{
#if defined(WIN32) && defined(_DEBUG)
    // Detecting memory leaks using CRT dbg
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef WIN32
    // To force the windows console to use UTF-8 code page.
    ::SetConsoleOutputCP(CP_UTF8);

    // Set console font to Lucida Console to fix issue on windows 7
    HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX font = { sizeof(font) };
    ::GetCurrentConsoleFontEx(hOut, FALSE, &font);
    ::wcscpy_s(font.FaceName, L"Lucida Console");
    ::SetCurrentConsoleFontEx(hOut, FALSE, &font);

    // Disable quick edit mode of console on Windows 10
    DWORD prev_mode;
    HANDLE hInput;
    hInput = ::GetStdHandle(STD_INPUT_HANDLE);
    ::GetConsoleMode(hInput, &prev_mode);
    ::SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | (prev_mode & ~ENABLE_QUICK_EDIT_MODE));
#endif
    return true;
}
} // namespace JUtils
