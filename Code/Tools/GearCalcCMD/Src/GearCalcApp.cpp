//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//

#include "JUtils/pch.h"

#include "JUtils/App.h"
#include "JUtils/Main.h"
#include "JUtils/Utils.h"

#define MAX_FRACTION_DIGITS_TO_PRINT 2
#include <iomanip>

#include "UserData.h"

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
        std::cout << u8"请输入r计算, q退出, c清屏: " << std::endl;
        std::cin >> input;

        switch (input)
        {
        case 'r':
        {
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

        GearFileData gearFileData;
        auto succeed = GearFileData::ReadFromJsonFile("XianqiData.json", m_errorStr, gearFileData);
        if (!succeed)
        {
            m_errorStr += u8"加载GearsData.json文件失败,请检查文件!";
            return;
        }
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
    std::string m_errorStr;
};

CMD_MAIN(GearCalcApp);
