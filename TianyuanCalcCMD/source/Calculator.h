//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "UserData.h"

#include <string>


#ifndef MAX_COMBO_SIZE_BITS
#define MAX_COMBO_SIZE_BITS 6
#endif // !MAX_COMBO_SIZE_BITS

namespace JUtils
{

class Calculator
{
public:
    enum class Solution
    {
        BestOfEachTarget= 0,
        OverallBest,

        Test
    };


    Calculator() {};

    bool Init(UnitScale::Values unitScale);

    bool LoadInputData(const char* fileName);
    bool LoadTargetData(const char* fileName);

    bool Run(ResultDataList& resultList, std::string& errorStr, Solution solution);

private:
    bool loadUserData(const char* fileName, UserDataList& dataList);

    UserDataList m_inputDataVec;
    UserDataList m_targetDataList;

    std::uint64_t m_unitScale = UnitScale::k_10K;
};

} // namespace JUtils