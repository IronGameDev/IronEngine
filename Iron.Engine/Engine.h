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
        filesystem
    };
};

struct engine_init_info {
    const char*     app_name;
    version         app_version;
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

constexpr static module_id fnv1a(const char* str) {
    module_id hash = 14695981039346656037ull;
    while (*str) {
        hash ^= static_cast<u8>(*str++);
        hash *= 1099511628211ull;
    }
    return hash;
}

constexpr module_id operator"" _mid(const char* str, size_t) {
    return fnv1a(str);
}

ENGINE_API result::code engine_initialize(const engine_init_info& init_info);
ENGINE_API void engine_shutdown();

/// <summary>
/// Get an engine module
/// </summary>
/// <param name="api">Requested API</param>
/// <param name="buffer_size">MUST be sizeof(vtable_module)</param>
/// <returns>vtable_module*</returns>
ENGINE_API void* get_module_vtable(engine_api::api api, u64 buffer_size);

////User Plugins???
//ENGINE_API result::code load_module(const char* path, engine_module* handle);
//ENGINE_API void unload_module(engine_module handle);

ENGINE_API version get_app_version();
}
