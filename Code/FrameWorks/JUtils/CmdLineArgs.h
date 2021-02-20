//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include <string>
#include <vector>

namespace JUtils
{
class CmdLineArgs
{
public:
    // Construct from char array, e.g. GetCommandLineA on Windows
    CmdLineArgs(const char* argsArray);

    // Construct from c-style main args
    CmdLineArgs(int argc, const char** argv);

    template <typename T>
    T GetArgValue(const char* cmdKey, const T& defaultValue) const
    {
        auto it_key = std::find(m_argsVector.begin(), m_argsVector.end(), cmdKey);
        if (it_key == m_argsVector.end())
            return defaultValue;

        auto it_value = it_key + 1;
        if (it_value != m_argsVector.end())
        {
            return FromString<T>(it_value->c_str());
        }
        return defaultValue;
    }
    template <typename T>
    T GetArgValue(const std::string& cmdKey, const T& defaultValue) const
    {
        return GetArgValue(cmdKey.c_str(), defaultValue);
    }

private:
    // Supported specializations are in cpp file
    template <typename T>
    static T FromString(const char* str);

private:
    std::vector<std::string> m_argsVector;
};

} // namespace JUtils
