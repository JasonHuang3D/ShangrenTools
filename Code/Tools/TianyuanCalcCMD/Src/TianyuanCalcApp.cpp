//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "JUtils/App.h"
#include "JUtils/Main.h"
#include "JUtils/Utils.h"

#include "TianyuanUserData.h"

#define MAX_FRACTION_DIGITS_TO_PRINT 2
#include "TianyuanCalculator.h"

#include <iomanip>

using namespace JUtils;
using namespace TianyuanCalc;

namespace
{
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
} // namespace

class TianyuanCalcApp : public CmdAppBase
{
public:
    TianyuanCalcApp(const CmdLineArgs& cmdLineArgs) : CmdAppBase(cmdLineArgs) {};

    int StartMainLoop() override
    {
        // Set print precision
        std::cout << std::fixed;
        std::cout << std::setprecision(MAX_FRACTION_DIGITS_TO_PRINT);

        return CmdAppBase::StartMainLoop();
    }

private:
    void OnIdleState() override { std::cout << u8"等待输入正确命令..." << std::endl; }
    AppState GetCurrentState()
    {
        char input;
        std::cout << u8"请输入r或t计算, q退出, c清屏: " << std::endl;
        std::cin >> input;

        m_calcSolution = Calculator::Solution::None;
        switch (input)
        {
        case 'r':
        {
            m_calcSolution = Calculator::Solution::BestOfEachTarget;
            return AppState::Running;
        }
        case 't':
        {
            m_calcSolution = Calculator::Solution::OverallBest;
            return AppState::Running;
        }
        case 'x':
        {
            m_calcSolution = Calculator::Solution::UnorderedTarget;
            return AppState::Running;
        }
        case 'q':
            return AppState::Exit;
        case 'c':
            return AppState::ClearScreen;
        default:
            return AppState::Idle;
        }
    }

    void OnRunningState() override
    {
        std::cout << u8"开始计算..." << std::endl;
        m_errorStr.clear();

        // Make sure m_calcSolution is set
        if (m_calcSolution == Calculator::Solution::None)
        {
            m_errorStr += "m_calcSolution is not set yet!\n";
            assert(false);
            return;
        }

        // Init calculator
        m_calculator.Init(UnitScale::k_10K);

        // Load user data
        if (!m_calculator.LoadInputData("inputData.txt", m_errorStr))
        {
            m_errorStr += u8"加载inputData.txt错误, 请检查文件及其内容!\n";
            return;
        }
        if (!m_calculator.LoadTargetData("targetData.txt", m_errorStr))
        {
            m_errorStr += u8"加载targetData.txt错误, 请检查文件及其内容!\n";
            return;
        }

        // Print a warning of choosing best overall solution
        if (m_calcSolution == Calculator::Solution::OverallBest)
        {
            std::cout << u8"已选择全局最优方案, 如时间太长, 请减少 InputData.txt 和 "
                         u8"targetData.txt 的数量. "
                      << std::endl;
        }

        // Print input out put size
        auto inputSize  = m_calculator.GetInputDataVec().GetList().size();
        auto targetSize = m_calculator.GetTargetDataVec().GetList().size();
        std::cout << u8"需要计算: " << inputSize << u8"个仙人, " << targetSize << u8"个目标."
                  << std::endl;

        // Run and record time spent
        ResultDataList resultList;
        Timer timer;
        bool isSucceed = m_calculator.Run(resultList, m_errorStr, m_calcSolution);
        auto timeSpent = timer.DurationInSec();
        if (!isSucceed)
            return;

        // Print the results
        {
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
                          << unitStr << u8",计算结果为: " << std::endl;
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
                          << FormatIntToFloat<double>(result.m_isExceeded ? result.m_difference : 0,
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
                      << FormatIntToFloat<double>(resultList.m_remainSum, resultList.m_unitScale)
                      << unitStr << std::endl;

            const auto& remainInputs = resultList.m_remainInputs;
            std::cout << u8"剩余仙人: " << remainInputs.size() << u8"个" << std::endl;
            for (int i = 0; i < remainInputs.size(); ++i)
            {
                auto& userData = remainInputs[i];
                PrintInputData(i + 1, userData, resultList.m_unitScale);
            }
        }

        std::cout << std::setprecision(3);
        std::cout << u8"计算时长: " << timeSpent << " 秒" << std::endl;
        std::cout << std::setprecision(MAX_FRACTION_DIGITS_TO_PRINT);
        PrintLargeSpace();
    }

    void OnExitState() override { std::cout << u8"关闭..." << std::endl; }
    void OnClearScreenState() override
    {
        std::cout << u8"清理屏幕..." << std::endl;
#ifdef WIN32
        ::system("cls");
#endif
    }
    bool ReportError() override
    {
        if (!m_errorStr.empty())
        {
            std::cout << m_errorStr << std::endl;
            std::cout << u8"计算有误, 请将截图和数据发给开发者, 谢谢." << std::endl;
            return true;
        }

        return false;
    }

private:
    Calculator::Solution m_calcSolution = Calculator::Solution::None;
    Calculator m_calculator;
    std::string m_errorStr;
};

CMD_MAIN(TianyuanCalcApp);
