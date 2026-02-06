#include <Iron.Core/Core.h>
#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>

#pragma comment(lib, "iron.core.lib")
#pragma comment(lib, "iron.engine.lib")

using namespace iron;

int main() {
    enable_log_include_path(option::enable);
    enable_log_level(log_level::debug, option::enable);
    enable_log_level(log_level::info, option::enable);
    enable_log_level(log_level::warning, option::enable);
    enable_log_level(log_level::error, option::enable);
    enable_log_level(log_level::fatal, option::enable);
    
    LOG_INFO("Started project");

    engine_init_info engine_info{};
    engine_info.app_name = "Engine Editor";
    engine_info.app_version.major = 12;
    engine_info.app_version.minor = 42;
    engine_info.app_version.patch = 948;

    result::code result{};
    result = engine_initialize(engine_info);
    if (!result::success(result)) {
        return -1;
    }

    vtable_windowing table{};
    engine_module window_module{};
    window_module.vtable = &table;
    load_module("Iron.Windowing.dll", &window_module);

    factory_windowing factory{ table };
    factory.initialize();

    factory.shutdown();

    unload_module(window_module);

    engine_shutdown();

    return 0;
}
