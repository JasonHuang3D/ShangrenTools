//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "pch.h"

#include "Calculator.h"
#include "UserData.h"
#include "Utils.h"

#include <iomanip>

#if defined(WIN32) && defined(M_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // (WIN32) && defined (M_DEBUG)

enum class AppState : std::uint8_t
{
    Idle = 0,
    ClearScreen,
    Calculating,
    Exit
};

void PrintLargeSpace()
{
    std::cout << "=========================================" << std::endl << std::endl;
}
void PrintSmallSpace()
{
    std::cout << "-----------------------------------------" << std::endl;
}

AppState GetCurrentState()
{
    char input;
    std::cout << u8"请输入r计算, q退出, c清屏: " << std::endl;
    std::cin >> input;

    auto outState = AppState::Idle;
    switch (input)
    {
    case 'r':
        outState = AppState::Calculating;
        break;
    case 'q':
        outState = AppState::Exit;
        break;
    case 'c':
        outState = AppState::ClearScreen;
        break;
    default:
        outState = AppState::Idle;
        break;
    }

    return outState;
}

AppState g_state = AppState::Idle;

int main(int argc, char* argv[])
{
#if defined(WIN32) && defined(M_DEBUG)
    // Detecting memory leaks using CRT dbg
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    using namespace JUtils;

    // Set print precision
    std::cout << std::fixed;
    std::cout << std::setprecision(4);

    Calculator calculator;

    while (g_state != AppState::Exit)
    {
        g_state = GetCurrentState();

        switch (g_state)
        {
        case AppState::Idle:
            std::cout << u8"等待输入正确命令..." << std::endl;
            break;
        case AppState::Calculating:
        {
            std::cout << u8"开始计算..." << std::endl;

            // Load user data
            if (!calculator.LoadInputData("inputData.txt", UnitScale::k10K))
            {
                std::cout << u8"加载inputData.txt错误, 请检查文件及其内容!" << std::endl;
                break;
            }
            if (!calculator.LoadTargetData("targetData.txt", UnitScale::k10K))
            {
                std::cout << u8"加载targetData.txt错误, 请检查文件及其内容!" << std::endl;
                break;
            }

            // Run
            ResultDataList resultList;
            std::string errorStr;
            if (calculator.Run(resultList, errorStr))
            {
                // Print the results
                auto unitStr = UnitScale::GetUnitStr(resultList.m_unitScale);

                std::cout << u8"计算结果:" << std::endl;
                PrintLargeSpace();

                auto& resultVec = resultList.m_list;
                for (int i = 0; i < resultVec.size(); ++i)
                {
                    auto& result = resultVec[i];

                    std::cout << u8"目标" << i + 1 << ": " << result.m_pTarget->GetDesc()
                              << u8", 需求: "
                              << FormatIntToFloat<double>(
                                     result.m_pTarget->GetData(), resultList.m_unitScale)
                              << u8",计算结果为: " << std::endl;
                    PrintSmallSpace();

                    auto& combination = result.m_combination;
                    for (int j = 0; j < combination.size(); ++j)
                    {
                        auto& userData = combination[j];
                        std::cout << u8"仙人" << j + 1 << ": " << userData->GetDesc() << u8", 战力:"
                                  << FormatIntToFloat<double>(
                                         userData->GetData(), resultList.m_unitScale)
                                  << unitStr << std::endl;
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
                PrintLargeSpace();
            }
            else
            {
                std::cout << errorStr;
                break;
            }

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