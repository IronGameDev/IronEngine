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
engine_initialize(const engine_init_info& init_info) {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    g_app_version = init_info.app_version;

    LOG_INFO("Initializing engine, App=%s Version=%u:%u:%u",
        init_info.app_name,
        init_info.app_version.major,
        init_info.app_version.minor,
        init_info.app_version.patch);

    return result::ok;
}

void
engine_shutdown() {
}

void*
get_module_vtable(engine_api::api api, u64 buffer_size)
{
    switch (api)
    {
    case engine_api::windowing:
        return g_context.load_and_get_vtable("Iron.Windowing.dll", buffer_size);
    case engine_api::renderer:
        break;
    case engine_api::input:
        return g_context.load_and_get_vtable("Iron.Input.dll", buffer_size);
    case engine_api::audio:
        break;
    case engine_api::filesystem:
        break;
    default:
        break;
    }

    return nullptr;
}

version
get_app_version() {
    return g_app_version;
}
}
