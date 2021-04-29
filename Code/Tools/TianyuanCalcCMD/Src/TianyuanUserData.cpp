//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "TianyuanUserData.h"

#include "JUtils/Utils.h"

#include <fstream>
#include <sstream>

using namespace JUtils;

namespace TianyuanCalc
{

bool UserDataList::ReadFromFile(
    const char* fileName, std::uint64_t uintScale, std::string& errorStr, UserDataList& outList)
{
    if (isCharPtrEmpty(fileName))
        return false;

    // Init out list
    outList.m_unitScale = uintScale;
    auto& list          = outList.m_list;
    list.clear();

    std::ifstream fileStream(fileName);
    // Chinese locale
    fileStream.imbue(std::locale("zh_CN.UTF-8"));

    if (fileStream.is_open())
    {
        // Read fileStream by line
        std::string line;
        std::uint64_t currentLineNum = 1;
        while (std::getline(fileStream, line))
        {
            // Remove Bom if any
            RemoveBomFromString(line);

            // Skip empty line or commented line
            if (line.empty() || line[0] == '#')
            {
                ++currentLineNum;
                continue;
            }

            // Split line by "," or ";" or " "
            auto args           = TockenizeString<std::string>(line, ",;\t ");
            const auto argsSize = args.size();

            // Make sure we have desc and data
            if (argsSize >= 2)
            {
                std::uint32_t argIndex = 0;

                auto desc  = args[argIndex++].c_str();
                auto fData = std::stod(args[argIndex++]) * outList.m_unitScale;

                // Parse Amplifier
                if (argsSize > 3)
                {
                    auto amplifierKey = args[argIndex++];
                    if (amplifierKey == u8"增幅百分比")
                    {
                        auto amplifierValue = std::stod(args[argIndex++]);
                        fData += (fData * amplifierValue) * 0.01;
                    }
                }

                auto data = static_cast<std::uint64_t>(fData);
                list.emplace_back(desc, data);
            }
            else
            {
                errorStr += FormatString(
                    u8"文件: ", fileName, u8" 第 ", currentLineNum, u8" 行出错，请确保输入格式!\n");
                return false;
            }

            ++currentLineNum;
        }

        if (list.empty())
        {
            errorStr += FormatString(u8"文件: ", fileName, u8" 没有解析到任何有效条目数!\n");
            return false;
        }

        fileStream.close();
        return true;
    }
    else
    {
        errorStr += FormatString(u8"文件: ", fileName, u8" 无法打开，请检查文件名和路径!\n");
        return false;
    }
}

bool ResultData::isFinished() const
{
    if (m_combination.empty())
        return false;

    if (m_difference == 0)
        return !m_isExceeded;
    else
        return m_isExceeded;
}
} // namespace TianyuanCalc
