#pragma once
#include <Iron.Core/Core.h>

#ifdef ENGINE_EXPORT
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

namespace iron {
typedef u64 module_id;

struct engine_api {
    enum api : u32 {
        windowing = 0,
        renderer,
        input,
        audio,
        filesystem,
        count
    };
};

struct engine_init_info {
    const char*         app_name;
    version             app_version;
    s32                 argc;
    char**              argv;
    option::value       headless;

    struct {
        u32             width{ 1024 };
        u32             height{ 768 };
        const char*     title{ "Iron Game" };
        option::value   fullscreen{ false };
    } window;
};

struct engine_module {
    u64             id;
    void*           vtable;
    void*           library;

    constexpr static inline u64 hash(const char* name) noexcept {
        u64 hash{ 0x468a3276cf9809ed };

        while (*name != '\0') {
            hash ^= (u64)*name + 0x83987239870acefd;
            ++name;
        }

        return hash;
    }

    constexpr static inline bool is_valid(const engine_module& m) {
        return m.library && m.vtable;
    }

    constexpr static inline bool is_loaded(const engine_module& m) {
        return m.library;
    }
};

class application {
public:
    virtual ~application() = default;

    //Before window / renderer creation
    //Will only run in non-headless mode
    virtual result::code pre_initialize() = 0;
    
    //After window / renderer creation
    //Should be used with headless mode
    virtual result::code post_initialize() = 0;
    
    virtual void frame() = 0;
    virtual void shutdown() = 0;
};

/// <summary>
/// Starts and runs the engine
/// </summary>
/// <param name="init_info"></param>
/// <param name="app"></param>
/// <returns></returns>
ENGINE_API result::code run_engine(const engine_init_info& init_info, application* app);

/// <summary>
/// This is how the user notifies the engine to stop running
/// </summary>
/// <returns></returns>
ENGINE_API void request_quit();

/// <summary>
/// Get an engine module
/// NEVER call destroy on these factories, this will cause UB
/// </summary>
/// <param name="api">Requested API</param>
/// <returns>initialized factory_module*</returns>
ENGINE_API void* const get_engine_api(engine_api::api api);

////User Plugins???
//ENGINE_API result::code load_module(const char* path, engine_module* handle);
//ENGINE_API void unload_module(engine_module handle);

//Deprecated!
ENGINE_API version get_app_version();
}
