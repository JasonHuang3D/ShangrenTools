//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "TianyuanUserData.h"

#include <string>

// This is used for max combination size of each calculation. 32 means 2 ^ 32 combinations is
// allowed in memory.
#ifndef MAX_COMB_NUM_BIT
#define MAX_COMB_NUM_BIT 32
#endif // !MAX_COMB_NUM_BIT

namespace TianyuanCalc
{

class Calculator
{
public:
    enum class Solution
    {
        None = 0,
        BestOfEachTarget,
        OverallBest,

        Test
    };

    Calculator() {};

    bool Init(UnitScale::Values unitScale);

    bool LoadInputData(const char* fileName, std::string& errorStr);
    bool LoadTargetData(const char* fileName, std::string& errorStr);

    bool Run(ResultDataList& resultList, std::string& errorStr, Solution solution);

    const UserDataList& GetInputDataVec() const { return m_inputDataVec; }
    const UserDataList& GetTargetDataVec() const { return m_targetDataList; }

private:
    bool loadUserData(const char* fileName, std::string& errorStr, UserDataList& dataList);

    UserDataList m_inputDataVec;
    UserDataList m_targetDataList;

    std::uint64_t m_unitScale = UnitScale::k_10K;
};

} // namespace TianyuanCalc