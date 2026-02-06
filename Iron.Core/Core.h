#pragma once

#if !(defined(_WIN32) || defined(_WIN64))
#error "Non-Windows platforms are not supported!"
#endif

#ifdef CORE_EXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef float f32;

struct option {
    enum value : u8 {
        disable = 0,
        enable = 1,
    };

    constexpr static inline bool get(value v) {
        return v == enable;
    }
};

struct version {
    u32         major : 11;
    u32         minor : 11;
    u32         patch : 10;

    version() = default;
    constexpr version(u32 vmajor, u32 vminor, u32 vpatch)
        : major(vmajor), minor(vminor), patch(vpatch) {
    }

    constexpr static inline u32 pack(const version& v) {
        return v.major << 21 | v.minor << 10 | v.patch;
    }

    constexpr static inline version unpack(u32 v) {
        version r{};
        r.major = (v >> 21) & 0x7ff;
        r.minor = (v >> 10) & 0x7ff;
        r.patch = v & 0x3ff;
        return r;
    }
};

#ifdef _DEBUG
#define LOG_DEBUG(x, ...) ::iron::log(::iron::log_level::debug, __FILE__, __LINE__, x, __VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif
#define LOG_INFO(x, ...) ::iron::log(::iron::log_level::info, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_WARNING(x, ...) ::iron::log(::iron::log_level::warning, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_ERROR(x, ...) ::iron::log(::iron::log_level::error, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_FATAL(x, ...) ::iron::log(::iron::log_level::fatal, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_RESULT(x) ::iron::log_error(x, __FILE__, __LINE__)

#ifndef INVALID_TABLE
#define INVALID_TABLE(func) LOG_ERROR("Vtable function %s() was called, but not implemented!", #func);
#define CHECK_TABLE(func) if(!m_table.func) {                                                                   \
LOG_WARNING("Failed to get vtable function %s(), this is fine as long as the user does not call this function",   \
#func); }
#endif

namespace iron {
struct result {
    enum code : u32 {
        ok = 0,

        e_nomemory,
        e_nullptr,
        e_invalidarg,
        e_incomplete,
        e_createwindow,
        e_loadlibrary,
        e_getvtable,
        
        count,
    };

    constexpr static inline bool success(code c) {
        return c == ok;
    }

    constexpr static inline bool fail(code c) {
        return c != ok;
    }
};

struct log_level {
    enum level : u32 {
        debug = 0,
        info,
        warning,
        error,
        fatal,
        count,
    };
};

CORE_API void* mem_alloc(size_t size);
CORE_API void mem_free(void* block);
CORE_API void mem_set(void* dst, u8 value, size_t size);
CORE_API void mem_copy(void* dst, const void* src, size_t size);
CORE_API void mem_copy_s(void* dst, const void* src, size_t dst_size, size_t src_size);

CORE_API void log(log_level::level level, const char* file, int line, const char* msg, ...);
CORE_API void enable_log_level(log_level::level level, option::value enable);
CORE_API void enable_log_include_path(option::value enable);
CORE_API void log_error(result::code code, const char* file, int line);
}
