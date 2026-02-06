#include <Iron.Engine/Engine.h>
#include <Iron.Core/Core.h>

#include <crtdbg.h>

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

version
get_app_version()
{
    return g_app_version;
}
}
