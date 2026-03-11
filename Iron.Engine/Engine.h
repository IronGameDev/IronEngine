#pragma once
#include <Iron.Core/Core.h>

#ifdef ENGINE_EXPORT
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

namespace Iron {
typedef u64 ModuleId;

struct EngineAPI {
    enum Api : u32 {
        Windowing = 0,
        Renderer,
        Input,
        AssetCompiler,
        Audio,
        Filesystem,
        Count
    };
};

struct EngineInitInfo {
    const char*     AppName;
    Version         AppVersion;
    s32             ArgC;
    char**          ArgV;
    bool            Headless;

    struct {
        u32             Width{ 1024 };
        u32             Height{ 768 };
        bool            Fullscreen{ false };
    } Window;
};

class Application {
public:
    virtual ~Application() = default;

    // Before window / renderer creation
    // Will only run in non-headless mode
    virtual Result::Code PreInitialize() = 0;

    // After window / renderer creation
    // Should be used with headless mode
    virtual Result::Code PostInitialize() = 0;

    virtual void Frame() = 0;
    virtual void Shutdown() = 0;
};

/// Starts and runs the engine
ENGINE_API Result::Code RunEngine(const EngineInitInfo& InitInfo, Application* App);

/// This is how the user notifies the engine to stop running
ENGINE_API void RequestQuit();

/// Get an engine module
/// NEVER call destroy on these factories, this will cause UB
ENGINE_API void* const GetEngineAPI(EngineAPI::Api Api);

ENGINE_API IObjectBase* LoadPlugin(const char* dllPath);
ENGINE_API void UnloadPlugin(const char* dllPath);

////User Plugins???
//ENGINE_API Result::Code LoadModule(const char* Path, EngineModule* Handle);
//ENGINE_API void UnloadModule(EngineModule Handle);

// Deprecated!
ENGINE_API Version GetAppVersion();
}
