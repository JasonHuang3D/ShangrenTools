//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "UserData.h"

#include "JUtils/Utils.h"

#include <fstream>
#include <sstream>

using namespace JUtils;

namespace TianyuanCalc
{

bool UserDataList::ReadFromFile(
    const char* fileName, std::uint64_t uintScale, UserDataList& outList)
{
    // Init out list
    outList.m_unitScale = uintScale;
    auto& list          = outList.m_list;
    list.clear();

    std::ifstream file(fileName);
    // Chinese locale
    file.imbue(std::locale("zh_CN.UTF-8"));

    if (file.is_open())
    {
        // Read file by line
        std::string line;
        while (std::getline(file, line))
        {
            // Skip empty line or commented line
            if (line.empty() || line[0] == '#')
                continue;

            // Splite line by "," or ";" or " "
            auto args = TockenizeString<std::string>(line, ",; ");

            // Make sure we have desc and data
            if (args.size() >= 2)
            {
                auto desc  = args[0].c_str();
                auto fData = std::stof(args[1]) * outList.m_unitScale;
                auto data  = static_cast<std::uint64_t>(fData);

                list.emplace_back(desc, data);
            }
            else
            {
                return false;
            }
        }

        file.close();
        return true;
    }

    return false;
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
