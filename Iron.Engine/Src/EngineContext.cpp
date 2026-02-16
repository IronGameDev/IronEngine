#include <Iron.Engine/Src/EngineContext.h>

#include <Windows.h>

namespace Iron {
namespace {
constexpr static const char* g_ModuleNames[EngineAPI::Count]{
    "Iron.Windowing.dll",
    "Iron.Renderer.dll",
    "Iron.Input.dll",
    "Iron.Audio.dll",
    "Iron.Filesystem.dll"
};

std::filesystem::path
GetExePath()
{
    char Path[MAX_PATH];
    const u32 Length{ GetModuleFileNameA(0, &Path[0], MAX_PATH) };
    if (!Length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};
    return std::filesystem::path(Path).remove_filename();
}
} // anonymous namespace

EngineContext g_Context{};

EngineContext::EngineContext()
    : m_LogEnableDebug(true),
    m_LogEnableInfo(true),
    m_LogEnableWarning(true),
    m_LogEnableError(true),
    m_LogEnableFatal(true),
    m_LogEnableFilename(true),
    m_Headless(true),
    m_Running(false)
{
    m_EngineDir = GetExePath();

    std::filesystem::path ConfigPath{ "D:\\code\\IronEngine\\" };
    ConfigPath.append("settings.ini");
    ConfigFile Config{};
    if (Result::Success(Config.Load(ConfigPath.string().c_str()))) {
        m_LogEnableDebug = atoi(Config.Get("engine.log", "enable_debug", "1"));
        m_LogEnableInfo = atoi(Config.Get("engine.log", "enable_info", "1"));
        m_LogEnableWarning = atoi(Config.Get("engine.log", "enable_warning", "1"));
        m_LogEnableError = atoi(Config.Get("engine.log", "enable_error", "1"));
        m_LogEnableFatal = atoi(Config.Get("engine.log", "enable_fatal", "1"));
        m_LogEnableFilename = atoi(Config.Get("engine.log", "enable_filename", "1"));
    }

    EnableLogLevel(LogLevel::Debug, m_LogEnableDebug ? Option::Enable : Option::Disable);
    EnableLogLevel(LogLevel::Info, m_LogEnableInfo ? Option::Enable : Option::Disable);
    EnableLogLevel(LogLevel::Warning, m_LogEnableWarning ? Option::Enable : Option::Disable);
    EnableLogLevel(LogLevel::Error, m_LogEnableError ? Option::Enable : Option::Disable);
    EnableLogLevel(LogLevel::Fatal, m_LogEnableFatal ? Option::Enable : Option::Disable);
    EnableLogIncludePath(m_LogEnableFilename ? Option::Enable : Option::Disable);
}

EngineContext::~EngineContext() {
    std::filesystem::path ConfigPath{ "D:\\code\\IronEngine\\" };
    ConfigPath.append("settings.ini");
    ConfigFile Config{};
    Config.Set("engine.log", "enable_debug", std::to_string(m_LogEnableDebug).c_str());
    Config.Set("engine.log", "enable_info", std::to_string(m_LogEnableInfo).c_str());
    Config.Set("engine.log", "enable_warning", std::to_string(m_LogEnableWarning).c_str());
    Config.Set("engine.log", "enable_error", std::to_string(m_LogEnableError).c_str());
    Config.Set("engine.log", "enable_fatal", std::to_string(m_LogEnableFatal).c_str());
    Config.Set("engine.log", "enable_filename", std::to_string(m_LogEnableFilename).c_str());

    if (Result::Fail(Config.Save(ConfigPath.string().c_str()))) {
        LOG_ERROR("Failed to save engine config settings!");
    }

    Reset();
}

Result::Code
EngineContext::Run(const EngineInitInfo& Info, Application* const App)
{
    m_InitInfo = Info;
    m_Headless = Option::Get(Info.Headless);

    if (!App) {
        LOG_FATAL("No application interface!");
        return Result::ENointerface;
    }

    Result::Code Res{ Result::Ok };

    if (!m_Headless) {
        m_WindowFactory = {
            (Window::IWindowFactory*)LoadAndGetFactory(
                g_ModuleNames[EngineAPI::Windowing])
        };
        
        Res = App->PreInitialize();
        if (Result::Fail(Res)) {
            App->Shutdown();
            return Res;
        }

        Window::WindowInitInfo WindowInfo{};
        WindowInfo.Width = Info.Window.Width;
        WindowInfo.Height = Info.Window.Height;
        WindowInfo.Title = Info.Window.Title;
        WindowInfo.Fullscreen = Info.Window.Fullscreen;

        Res = m_WindowFactory->OpenWindow(WindowInfo, &m_MainWindow);
        if (Result::Fail(Res)) {
            LOG_FATAL("A window was requested, but could not be opened!");
            return Res;
        }

        m_MainWindow->SetIcon("D:\\code\\IronEngine\\iron.ico", { 32, 32 });
        m_MainWindow->SetBorderColor({ 0.2f, 0.2f, 0.2f });
        m_MainWindow->SetTitleColor({ 0.9f, 0.9f, 1.f });
    }

    Res = App->PostInitialize();
    if (Result::Fail(Res)) {
        App->Shutdown();
        return Res;
    }

    m_Running = true;

    while (m_Running.load()) {
        App->Frame();

        m_MainWindow->PumpMessages();
        if (!m_MainWindow->IsOpen()) {
            m_Running = false;
        }
    }

    App->Shutdown();

    if (!m_Headless) {
        SafeRelease(m_MainWindow);
    }

    return Result::Ok;
}

IObjectBase* const
EngineContext::LoadAndGetFactory(const char* DllName)
{
    std::filesystem::path FullPath{ m_EngineDir };
    FullPath.append(DllName);
    const u64 Id{ Fnv1A(DllName) };
    const Result::Code Res{ m_Modules.LoadModule(FullPath.string().c_str(), Id) };
    if (Result::Fail(Res)) {
        LOG_RESULT(Res);
        return nullptr;
    }

    return m_Modules.GetFactory(Id);
}

IObjectBase* const
EngineContext::GetEngineAPI(EngineAPI::Api Api) {
    switch (Api)
    {
    case EngineAPI::Windowing:
        return m_WindowFactory;
    case EngineAPI::Renderer:
    case EngineAPI::Input:
    case EngineAPI::Audio:
    case EngineAPI::Filesystem:
    default:
        break;
    }

    return nullptr;
}

void
EngineContext::ParseCommandArgs(s32 ArgC, char** ArgV) {
    for (s32 I{ 0 }; I < ArgC; ++I) {
        LOG_INFO("Parsed command argument %s", ArgV[I]);
        m_CmdArgs.emplace_back(ArgV[I]);
    }
}

void
EngineContext::Reset() {
    for (u32 I{ 0 }; I < EngineAPI::Count; ++I) {
        m_Modules.UnloadModule(Fnv1A(g_ModuleNames[I]));
    }

    m_Modules.Reset();
    m_CmdArgs = {};
}
}
