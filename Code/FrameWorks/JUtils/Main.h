//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "CmdLineArgs.h"
#include <memory>

namespace JUtils
{
bool ConfigPlatformCMD();
} // namespace JUtils

#define CMD_MAIN(appClass)                                                                         \
    int main(int argc, const char* argv[])                                                         \
    {                                                                                              \
        if (!ConfigPlatformCMD())                                                                  \
            return -1;                                                                             \
        JUtils::CmdLineArgs cmdArgs(argc, argv);                                                   \
        auto pApp = std::make_unique<appClass>(cmdArgs);                                           \
        return pApp->StartMainLoop();                                                              \
    }