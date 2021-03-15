//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "GearUserData.h"

namespace GearCalc
{

class Calculator
{
public:
    enum class Solution :std::uint8_t
    {
        BestXianRenSumProp,

        BestGlobalSumLiNian,

        BestChanJing,
        BestChanNeng,

        Test,

        NumSolutions,
        None
    };

    Calculator() {};

    bool Init(const char* XianjieFile, const char* XianqiFile, std::string& errorStr);
    bool Run(std::string& errorStr, Solution solution);

private:
    XianJieFileData m_xianJieFileData;
    XianQiFileData m_xianQiFileData;

    bool m_isInitialized = false;
};
} // namespace GearCalc