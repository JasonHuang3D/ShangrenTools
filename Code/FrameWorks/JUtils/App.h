//
// Author: Jason Huang(jasonhuang1988@gmail.com) 2021
//
#pragma once

#include "CmdLineArgs.h"

namespace JUtils
{

class AppBase
{
public:
    AppBase(const CmdLineArgs& cmdLineArgs);
    virtual ~AppBase();

    virtual int StartMainLoop() = 0;

protected:
    const CmdLineArgs& m_cmdLineArgs;
};

class CmdAppBase : AppBase
{
public:
    enum class AppState : std::uint8_t
    {
        Idle = 0,
        Running,
        Exit,
        ClearScreen
    };

public:
    CmdAppBase(const CmdLineArgs& cmdLineArgs);
    virtual ~CmdAppBase();

    virtual int StartMainLoop() override;

protected:
    virtual AppState GetCurrentState() = 0;

    virtual void OnIdleState() = 0;
    virtual void OnRunningState() = 0;
    virtual void OnExitState() = 0;
    virtual void OnClearScreenState() = 0;

    virtual bool ReportError() = 0;

};
} // namespace JUtils