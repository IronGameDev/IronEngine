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

struct engine_module {
    void*           vtable;
    void*           library;

    constexpr static inline bool is_valid(const engine_module& m) {
        return m.library && m.vtable;
    }
};

ENGINE_API result::code engine_initialize(const engine_init_info& init_info);
ENGINE_API void engine_shutdown();

ENGINE_API result::code load_module(const char* path, engine_module* handle);
ENGINE_API void unload_module(engine_module handle);

ENGINE_API version get_app_version();
}
