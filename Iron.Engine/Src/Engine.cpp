#include <Iron.Engine/Engine.h>
#include <Iron.Core/Core.h>

#include <crtdbg.h>
#include <Windows.h>
#include <filesystem>

namespace iron {
namespace {
std::string
get_current_directory()
{
    char path[MAX_PATH];
    const u32 length{ GetModuleFileNameA(0, &path[0], MAX_PATH) };
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};
    return std::filesystem::path(path).parent_path().string().append("\\");
}

version             g_app_version{};
engine_config       g_config{};
}//anonymous namespace

result::code
engine_initialize(const engine_init_info& init_info) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    g_app_version = init_info.app_version;

    LOG_INFO("Initializing engine, App=%s Version=%u:%u:%u",
        init_info.app_name,
        init_info.app_version.major,
        init_info.app_version.minor,
        init_info.app_version.patch);

    const std::string current_dir{ get_current_directory() };
    strcpy_s(g_config.engine_path, IRON_MAX_PATH, current_dir.c_str());

    return result::ok;
}

void
engine_shutdown() {
}

version
get_app_version() {
    return g_app_version;
}

engine_config* get_config() {
    return &g_config;
}
}
