//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "UserData.h"

#include <string>

namespace JUtils
{

class Calculator
{
public:
    Calculator() {};

    bool LoadInputData(const char* fileName, std::uint64_t unitScale);
    bool LoadTargetData(const char* fileName, std::uint64_t unitScale);

    bool Run(ResultDataList& resultList, std::string& errorStr);

private:

    UserDataList m_inputDataVec;
    UserDataList m_targetDataList;
};

} // namespace JUtils