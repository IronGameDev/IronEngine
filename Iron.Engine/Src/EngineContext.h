#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.Engine/Src/Modules/Modules.h>
#include <Iron.Windowing/Windowing.h>

#include <filesystem>

namespace Iron {

class EngineContext {
public:
    EngineContext();
    ~EngineContext();

    Result::Code Run(const EngineInitInfo& Info, Application* const App);

    ModuleManager               m_Modules{};

    std::vector<std::string>    m_CmdArgs{};
    EngineInitInfo              m_InitInfo{};

    std::filesystem::path       m_EngineDir{};

    Window::IWindowFactory*     m_WindowFactory{};
    Window::IWindow*            m_MainWindow{};

    std::atomic<bool>           m_Running{};

public:
    IObjectBase* const LoadAndGetFactory(const char* DllName);
    IObjectBase* const GetEngineAPI(EngineAPI::Api Api);
    void ParseCommandArgs(s32 ArgC, char** ArgV);

    void Reset();

public:
    // Log config
    bool m_LogEnableDebug : 1;
    bool m_LogEnableInfo : 1;
    bool m_LogEnableWarning : 1;
    bool m_LogEnableError : 1;
    bool m_LogEnableFatal : 1;
    bool m_LogEnableFilename : 1;

    // Disable windowing and rendering
    bool m_Headless : 1;
};

extern EngineContext g_Context;
}
