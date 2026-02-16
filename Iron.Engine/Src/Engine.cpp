#include <Iron.Engine/Engine.h>
#include <Iron.Engine/Src/EngineContext.h>
#include <Iron.Core/Core.h>

#include <crtdbg.h>
#include <Windows.h>
#include <filesystem>

/// <summary>
/// TODO: Remove this file and refactor to EngineContext?
/// Or: use this file as translation code to EngineContext?
/// </summary>

namespace Iron {
namespace {

Version g_AppVersion{};

} // anonymous namespace

Result::Code
RunEngine(const EngineInitInfo& InitInfo, Application* const App) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    g_AppVersion = InitInfo.AppVersion;

    LOG_INFO("Initializing engine, App=%s Version=%u:%u:%u",
        InitInfo.AppName,
        InitInfo.AppVersion.Major,
        InitInfo.AppVersion.Minor,
        InitInfo.AppVersion.Patch);

    g_Context.ParseCommandArgs(InitInfo.ArgC, InitInfo.ArgV);

    return g_Context.Run(InitInfo, App);
}

void
RequestQuit() {
    g_Context.m_Running = false;
}

void* const
GetEngineAPI(EngineAPI::Api Api) {
    return g_Context.GetEngineAPI(Api);
}

Version
GetAppVersion() {
    return g_AppVersion;
}
}
