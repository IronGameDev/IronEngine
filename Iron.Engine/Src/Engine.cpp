#include <Iron.Engine/Engine.h>
#include <Iron.Engine/Src/EngineContext.h>
#include <Iron.Core/Core.h>

#include <crtdbg.h>
#include <Windows.h>
#include <filesystem>

/// <summary>
/// TODO: Remove this file and refactor to engine context?
/// Or: use this file as translation code to engine context?
/// </summary>

namespace iron {
namespace {
version             g_app_version{};
}//anonymous namespace

result::code
run_engine(const engine_init_info& init_info, application* const app) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    g_app_version = init_info.app_version;

    LOG_INFO("Initializing engine, App=%s Version=%u:%u:%u",
        init_info.app_name,
        init_info.app_version.major,
        init_info.app_version.minor,
        init_info.app_version.patch);

    g_context.parse_command_args(init_info.argc, init_info.argv);

    return g_context.run(init_info, app);
}

void
request_quit() {
    g_context.m_running = false;
}

void* const
get_engine_api(engine_api::api api) {
    return g_context.get_engine_api(api);
}

version
get_app_version() {
    return g_app_version;
}
}
