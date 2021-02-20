//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "CmdLineArgs.h"

namespace JUtils
{
CmdLineArgs::CmdLineArgs(const char* argsArray)
{
    assert(argsArray != nullptr);

    // Get char length
    size_t argCharLen                 = std::strlen(argsArray);
    static constexpr auto kDelimiters = " \n\r\t";
    static constexpr char kFence      = '\"';

    char* pCopy = new char[argCharLen + 1];
    char* ptr   = pCopy;
    std::memcpy(pCopy, argsArray, argCharLen + 1);

    char* pEnd = ptr + argCharLen;
    while (ptr < pEnd)
    {
        char* c = nullptr;

        // Skip white space
        while (*ptr && std::strchr(kDelimiters, *ptr))
        {
            ++ptr;
        }

        if (*ptr)
        {
            // check for fenced area
            if ((kFence == *ptr) && (0 != (c = std::strchr(++ptr, kFence))))
            {
                *c++ = 0;
                m_argsVector.emplace_back(ptr);
                ptr = c;
            }
            else if (0 != (c = std::strpbrk(ptr, kDelimiters)))
            {
                *c++ = 0;
                m_argsVector.emplace_back(ptr);
                ptr = c;
            }
            else
            {
                m_argsVector.emplace_back(ptr);
                break;
            }
        }
    }

    delete[] pCopy;
}

CmdLineArgs::CmdLineArgs(int argc, const char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        m_argsVector.emplace_back(argv[i]);
    }
}

template <>
const char* CmdLineArgs::FromString(const char* str)
{
    return str;
}
template <>
std::string CmdLineArgs::FromString(const char* str)
{
    return std::string(str);
}
template <>
int CmdLineArgs::FromString(const char* str)
{
    return std::atoi(str);
}
template <>
float CmdLineArgs::FromString(const char* str)
{
    return static_cast<float>(std::atof(str));
}
template <>
double CmdLineArgs::FromString(const char* str)
{
    return std::atof(str);
}
} // namespace JUtils
