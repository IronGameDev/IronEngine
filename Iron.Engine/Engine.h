#pragma once
#include <Iron.Core/Core.h>

#ifdef ENGINE_EXPORT
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

namespace iron {
struct engine_init_info {
    const char*     app_name;
    version         app_version;
};

struct engine_dependencies {
    bool            windowing : 1;
    bool            rendering : 1;
    bool            input : 1;
    bool            audio : 1;
    bool            filesystem : 1;
};

struct engine_config {
    engine_dependencies modules;
    char                engine_path[IRON_MAX_PATH];
};

struct engine_module {
    void*           vtable;
    void*           library;

    constexpr static inline bool is_valid(const engine_module& m) {
        return m.library && m.vtable;
    }
};

ENGINE_API result::code engine_initialize(const engine_init_info& init_info);
ENGINE_API void engine_shutdown();

//User Plugins???
ENGINE_API result::code load_module(const char* path, engine_module* handle);
ENGINE_API void unload_module(engine_module handle);

ENGINE_API version get_app_version();
ENGINE_API engine_config* get_config();
}
