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

    // Fix windows 7 utf-8 issue
    struct MBuf : public std::stringbuf
    {
        int sync()
        {
            ::fputs(this->str().c_str(), stdout);
            this->str("");
            return 0;
        }
    };
    ::setvbuf(stdout, nullptr, _IONBF, 0);
    static MBuf buf;
    std::cout.rdbuf(&buf);

    //std::cout << u8"Greek: αβγδ; German: Übergrößenträger" << std::endl;

    // Disable quick edit mode of console on Windows 10
    HANDLE hInput;
    DWORD prev_mode;
    hInput = ::GetStdHandle(STD_INPUT_HANDLE);
    ::GetConsoleMode(hInput, &prev_mode);
    ::SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | (prev_mode & ~ENABLE_QUICK_EDIT_MODE));
#endif
    return true;
}
} // namespace JUtils
