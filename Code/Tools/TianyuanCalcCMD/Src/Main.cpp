//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "JUtils/Utils.h"

#include "UserData.h"

#define MAX_FRACTION_DIGITS_TO_PRINT 2

#define MAX_INPUT_SIZE 64
#define MAX_COMB_NUM_BIT 32
#include "Calculator.h"

#include <iomanip>

#if defined(WIN32) && defined(M_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // (WIN32) && defined (M_DEBUG)

using namespace JUtils;
using namespace TianyuanCalc;

namespace
{
enum class AppState : std::uint8_t
{
    Idle = 0,
    ClearScreen,
    Calculating,
    Exit
};
struct State
{
    AppState appState                 = AppState::Idle;
    Calculator::Solution calcSolution = Calculator::Solution::None;
};
void PrintLargeSpace()
{
    std::cout << "=========================================" << std::endl << std::endl;
}
void PrintSmallSpace()
{
    std::cout << "-----------------------------------------" << std::endl;
}

void PrintInputData(std::uint32_t printIndex, const UserData* pUserData, std::uint64_t unitScale)
{
    if (!pUserData)
        return;

    std::cout << u8"仙人" << printIndex << ": " << pUserData->GetDesc() << u8", 战力:"
              << FormatIntToFloat<double>(pUserData->GetOriginalData(), unitScale)
              << UnitScale::GetUnitStr(unitScale) << std::endl;
}
void GetCurrentState(State& outState)
{
    char input;
    std::cout << u8"请输入r或t计算, q退出, c清屏: " << std::endl;
    std::cin >> input;

    outState.calcSolution = Calculator::Solution::None;
    switch (input)
    {
    case 'r':
        outState.appState     = AppState::Calculating;
        outState.calcSolution = Calculator::Solution::BestOfEachTarget;
        break;
    case 't':
        outState.appState     = AppState::Calculating;
        outState.calcSolution = Calculator::Solution::OverallBest;
        break;
    case 'q':
        outState.appState = AppState::Exit;
        break;
    case 'c':
        outState.appState = AppState::ClearScreen;
        break;
    default:
        outState.appState = AppState::Idle;
        break;
    }
}
} // namespace

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(_DEBUG)
    // Detecting memory leaks using CRT dbg
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef WIN32
    // To force the windows console to use UTF-8 code page.
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    // Set print precision
    std::cout << std::fixed;
    std::cout << std::setprecision(MAX_FRACTION_DIGITS_TO_PRINT);

    State currentState;
    Calculator calculator;

    while (currentState.appState != AppState::Exit)
    {
        GetCurrentState(currentState);

        switch (currentState.appState)
        {
        case AppState::Idle:
            std::cout << u8"等待输入正确命令..." << std::endl;
            break;
        case AppState::Calculating:
        {
            std::cout << u8"开始计算..." << std::endl;

            // Init calculator
            calculator.Init(UnitScale::k_10K);

            // Load user data
            if (!calculator.LoadInputData("inputData.txt"))
            {
                std::cout << u8"加载inputData.txt错误, 请检查文件及其内容!" << std::endl;
                break;
            }
            if (!calculator.LoadTargetData("targetData.txt"))
            {
                std::cout << u8"加载targetData.txt错误, 请检查文件及其内容!" << std::endl;
                break;
            }

            // Print a warning of choosing best overall solution
            if (currentState.calcSolution == Calculator::Solution::OverallBest)
            {
                std::cout << u8"已选择全局最优方案, 如时间太长, 请减少 InputData.txt 和 "
                             u8"targetData.txt 的数量. "
                          << std::endl;
            }

            // Print input out put size
            auto inputSize  = calculator.GetInputDataVec().GetList().size();
            auto targetSize = calculator.GetTargetDataVec().GetList().size();
            std::cout << u8"需要计算: " << inputSize << u8"个仙人, " << targetSize << u8"个目标."
                      << std::endl;

            // Run and record time spent
            ResultDataList resultList;
            std::string errorStr;
            Timer timer;
            bool isSucceed = calculator.Run(resultList, errorStr, currentState.calcSolution);
            auto timeSpent = timer.DurationInSec();

            if (isSucceed)
            {
                // Print the results
                auto unitStr = UnitScale::GetUnitStr(resultList.m_unitScale);

                std::cout << u8"计算结果:" << std::endl;
                PrintLargeSpace();

                auto& resultVec = resultList.m_selectedInputs;
                for (int i = 0; i < resultVec.size(); ++i)
                {
                    auto& result = resultVec[i];

                    std::cout << u8"目标" << i + 1 << ": " << result.m_pTarget->GetDesc()
                              << u8", 需求: "
                              << FormatIntToFloat<double>(
                                     result.m_pTarget->GetOriginalData(), resultList.m_unitScale)
                              << u8",计算结果为: " << std::endl;
                    PrintSmallSpace();

                    auto& combination = result.m_combination;
                    for (int j = 0; j < combination.size(); ++j)
                    {
                        auto& userData = combination[j];
                        PrintInputData(j + 1, userData, resultList.m_unitScale);
                    }
                    PrintSmallSpace();

                    std::cout << u8"战力总计: "
                              << FormatIntToFloat<double>(result.m_sum, resultList.m_unitScale)
                              << unitStr << std::endl;
                    std::cout << u8"溢出总计: "
                              << FormatIntToFloat<double>(
                                     result.m_isExceeded ? result.m_difference : 0,
                                     resultList.m_unitScale)
                              << unitStr << std::endl;
                    std::cout << u8"剩余总计: "
                              << FormatIntToFloat<double>(
                                     !result.m_isExceeded ? result.m_difference : 0,
                                     resultList.m_unitScale)
                              << unitStr << std::endl;
                    PrintLargeSpace();
                }

                std::cout << u8"统计:" << std::endl;
                PrintSmallSpace();

                std::cout << u8"总共完成: " << resultList.m_numfinished << u8" 个目标" << std::endl;
                std::cout << u8"战力总计: "
                          << FormatIntToFloat<double>(resultList.m_combiSum, resultList.m_unitScale)
                          << unitStr << std::endl;
                std::cout << u8"溢出总计: "
                          << FormatIntToFloat<double>(resultList.m_exeedSum, resultList.m_unitScale)
                          << unitStr << std::endl;
                std::cout << u8"剩余总计: "
                          << FormatIntToFloat<double>(
                                 resultList.m_remainSum, resultList.m_unitScale)
                          << unitStr << std::endl;

                const auto& remainInputs = resultList.m_remainInputs;
                std::cout << u8"剩余仙人: " << remainInputs.size() << u8"个" << std::endl;
                for (int i = 0; i < remainInputs.size(); ++i)
                {
                    auto& userData = remainInputs[i];
                    PrintInputData(i + 1, userData, resultList.m_unitScale);
                }
            }
            else
            {
                std::cout << u8"计算有误, 请将截图和数据发给开发者, 谢谢." << std::endl;
                std::cout << errorStr << std::endl;
                break;
            }

            std::cout << std::setprecision(3);
            std::cout << u8"计算时长: " << timeSpent << " 秒" << std::endl;
            std::cout << std::setprecision(MAX_FRACTION_DIGITS_TO_PRINT);
            PrintLargeSpace();
            break;
        }
        case AppState::ClearScreen:
        {
            std::cout << u8"清理屏幕..." << std::endl;
#ifdef WIN32
            ::system("cls");
#endif
            break;
        }
        case AppState::Exit:
            std::cout << u8"关闭..." << std::endl;
            break;
        default:
            break;
        }
    }
    return 0;
}