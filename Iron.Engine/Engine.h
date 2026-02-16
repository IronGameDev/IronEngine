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
    Option::Value   Headless;

    struct {
        u32             Width{ 1024 };
        u32             Height{ 768 };
        const char*     Title{ "Iron Game" };
        Option::Value   Fullscreen{ false };
    } Window;
};

struct EngineModule {
    u64             Id;
    void*           Library;
    IObjectBase*    Factory;

    constexpr static inline u64 Hash(const char* Name) noexcept {
        u64 Hash{ 0x468a3276cf9809ed };

        while (*Name != '\0') {
            Hash ^= (u64)*Name + 0x83987239870acefd;
            ++Name;
        }

        return Hash;
    }

    constexpr static inline bool IsValid(const EngineModule& M) {
        return M.Library && M.Factory;
    }

    constexpr static inline bool IsLoaded(const EngineModule& M) {
        return M.Library;
    }
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

////User Plugins???
//ENGINE_API Result::Code LoadModule(const char* Path, EngineModule* Handle);
//ENGINE_API void UnloadModule(EngineModule Handle);

// Deprecated!
ENGINE_API Version GetAppVersion();
}
