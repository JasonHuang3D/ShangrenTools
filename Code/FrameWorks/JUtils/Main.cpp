//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Main.h"

#if defined(WIN32) && defined(M_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // (WIN32) && defined (M_DEBUG)

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
#endif
    return true;
}
} // namespace JUtils
