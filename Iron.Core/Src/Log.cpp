#include <Iron.Core/Core.h>

#include <mutex>
#include <stdarg.h>
#include <stdio.h>

namespace Iron {
namespace {
bool        g_EnabledLevels[LogLevel::Count]{};
bool        g_IncludeFile{ true };
std::mutex  g_LogMutex{};

constexpr static const char* g_Errors[Result::Count]{
    "Ok",
    "ENomemory",
    "ENullptr",
    "EInvalidarg",
    "EIncomplete",
    "ECreatewindow",
    "ELoadlibrary",
    "EGetvtable",
    "ESizemismatch",
    "ELoadfile",
    "EWritefile",
    "ENointerface",
    "ELoadIcon",
};

} // anonymous namespace


void
Log(LogLevel::Level Level, const char* File, int Line, const char* Msg, ...) {
    std::lock_guard Lock{ g_LogMutex };

    if (Level >= LogLevel::Count
        || !g_EnabledLevels[Level]) {
        return;
    }

    constexpr static const char* LevelText[]{
        "\x1b[36m[DEBUG]\x1b[0m",
        "\x1b[37m[INFO]\x1b[0m",
        "\x1b[33m[WARNING]\x1b[0m",
        "\x1b[31m[ERROR]\x1b[0m",
        "\x1b[34m[FATAL]\x1b[0m"
    };

    static_assert(_countof(LevelText) == LogLevel::Count, "Level text array size mismatch");

    va_list Args;
    va_start(Args, Msg);

    if (g_IncludeFile) {
        printf("%s %s:%i ", LevelText[(u32)Level], File, Line);
    }
    else {
        printf("%s ", LevelText[(u32)Level]);
    }

    vprintf(Msg, Args);
    printf("\n");

    va_end(Args);
}

void
EnableLogLevel(LogLevel::Level Level, bool Enable) {
    if (Level >= LogLevel::Count) {
        return;
    }

    std::lock_guard Lock{ g_LogMutex };
    g_EnabledLevels[Level] = Enable;
}

void
EnableLogIncludePath(bool  Enable) {
    std::lock_guard Lock{ g_LogMutex };
    g_IncludeFile = Enable;
}

void
LogError(Result::Code Code, const char* File, int Line)
{
    if (Code > Result::Count)
        return;

    if (Code == Result::Ok) {
        Log(LogLevel::Info, File, Line, g_Errors[Code]);
    }
    else {
        Log(LogLevel::Error, File, Line, g_Errors[Code]);
    }
}
}
