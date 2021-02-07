//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "UserData.h"

#include <string>

// This should not be greater than 64, as it is the max bits we can use. Otherwise by using
// std::bitset produces a lot of overheads.
#ifndef MAX_INPUT_SIZE
#define MAX_INPUT_SIZE 64
#endif // !MAX_COMBO_SIZE_BITS

// This is used for max combination size of each calculation. 32 means 2 ^ 32 combinations is
// allowed in memory.
#ifndef MAX_COMB_NUM_BIT
#define MAX_COMB_NUM_BIT 32
#endif // !MAX_COMB_NUM_BIT

namespace JUtils
{

class Calculator
{
public:
    enum class Solution
    {
        BestOfEachTarget = 0,
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