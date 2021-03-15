//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "JUtils/App.h"
#include "JUtils/Main.h"
#include "JUtils/Utils.h"

#define MAX_FRACTION_DIGITS_TO_PRINT 2
#include <iomanip>

#include "GearCalculator.h"

using namespace JUtils;
using namespace GearCalc;
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

} // namespace

class GearCalcApp : public CmdAppBase
{
public:
    GearCalcApp(const CmdLineArgs& cmdLineArgs) : CmdAppBase(cmdLineArgs) {};

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
        std::cout << u8"请输入数字1到9计算, q退出, c清屏: " << std::endl;
        std::cin >> input;

        m_currentSolution = Calculator::Solution::None;

        switch (input)
        {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        {
            int index = std::atoi(&input);
            --index;

            // Check the range against Calculator::Solution
            if (index < 0 || index >= static_cast<int>(Calculator::Solution::NumSolutions))
                return AppState::Idle;

            m_currentSolution = static_cast<Calculator::Solution>(index);
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
        std::cout << u8"开始计算";
        switch (m_currentSolution)
        {
        case GearCalc::Calculator::Solution::BestXianRenSumProp:
            std::cout << std::quoted(u8"最佳仙人总属性");
            break;
        case GearCalc::Calculator::Solution::BestGlobalSumLiNian:
            std::cout << std::quoted(u8"最佳宗门总力念");
            break;
        case GearCalc::Calculator::Solution::BestChanJing:
            std::cout << std::quoted(u8"最佳产仙晶");
            break;
        case GearCalc::Calculator::Solution::BestChanNeng:
            std::cout << std::quoted(u8"最佳产仙能");
            break;
        case GearCalc::Calculator::Solution::Test:
            std::cout << std::quoted(u8"测试");
            break;
        default:
            break;
        }
        std::cout <<"..."<< std::endl;

        m_errorStr.clear();

        if (!m_calculator.Init("XianjieData.json", "XianqiData.json", m_errorStr))
            return;

        if (!m_calculator.Run(m_errorStr, m_currentSolution))
            return;
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
    Calculator m_calculator;
    Calculator::Solution m_currentSolution = Calculator::Solution::None;

    std::string m_errorStr;
};

CMD_MAIN(GearCalcApp);
