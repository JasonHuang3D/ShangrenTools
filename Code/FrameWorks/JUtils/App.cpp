//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#include "pch.h"

#include "App.h"

namespace JUtils
{
AppBase::AppBase(const CmdLineArgs& cmdLineArgs) : m_cmdLineArgs(cmdLineArgs) {}
AppBase::~AppBase() {}

CmdAppBase::CmdAppBase(const CmdLineArgs& cmdLineArgs) : AppBase(cmdLineArgs) {}
CmdAppBase::~CmdAppBase() {}

int CmdAppBase::StartMainLoop()
{
    AppState currentState = AppState::Idle;

    while (currentState != AppState::Exit)
    {
        currentState = GetCurrentState();
        switch (currentState)
        {
        case AppState::Idle:
            OnIdleState();
            break;
        case AppState::Running:
            OnRunningState();
            break;
        case AppState::Exit:
            OnExitState();
            break;
        case AppState::ClearScreen:
            OnClearScreenState();
            break;
        default:
            break;
        }

        // Exit if we have any error
        if (ReportError())
        {
            OnExitState();
            return -1;
        }
    }

    return 0;
}

} // namespace JUtils