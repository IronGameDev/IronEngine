#include <Iron.Core/Core.h>

#include <mutex>
#include <stdarg.h>
#include <stdio.h>

namespace iron {
namespace {
bool                    g_enabled_levels[log_level::count]{};
bool                    g_include_file{ true };
std::mutex              g_log_mutex{};

constexpr static const char*    g_errrors[result::count] {
    "Success",
    "e_nomemory",
    "e_nullptr",
    "e_invalidarg",
    "e_incomplete",
    "e_createwindow",
    "e_loadlibrary",
    "e_getvtable"
};
}//anonymous namespace

void
log(log_level::level level, const char* file, int line, const char* msg, ...) {
    std::lock_guard lock{ g_log_mutex };

    if (level >= log_level::count
        || !g_enabled_levels[level]) {
        return;
    }

    constexpr static const char* level_text[]{
        "\x1b[36m[DEBUG]\x1b[0m",
        "\x1b[37m[INFO]\x1b[0m",
        "\x1b[33m[WARNING]\x1b[0m",
        "\x1b[31m[ERROR]\x1b[0m",
        "\x1b[34m[FATAL]\x1b[0m"
    };

    static_assert(_countof(level_text) == log_level::count, "Level text array size mismatch");

    va_list args;
    va_start(args, msg);

    if (g_include_file) {
        printf("%s %s:%i ", level_text[(u32)level], file, line);
    }
    else {
        printf("%s ", level_text[(u32)level]);
    }
    
    vprintf(msg, args);
    printf("\n");

    va_end(args);
}

void
enable_log_level(log_level::level level, option::value enable) {
    if (level >= log_level::count) {
        return;
    }

    std::lock_guard lock{ g_log_mutex };
    g_enabled_levels[level] = option::get(enable);
}

void
enable_log_include_path(option::value enable) {
    std::lock_guard lock{ g_log_mutex };
    g_include_file = option::get(enable);
}

void
log_error(result::code code, const char* file, int line)
{
    if (code > result::count)
        return;

    if (code == result::ok) {
        log(log_level::info, file, line, g_errrors[code]);
    }
    else {
        log(log_level::error, file, line, g_errrors[code]);
    }
}
}
