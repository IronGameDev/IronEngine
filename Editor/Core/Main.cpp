#include <Iron.Core/Core.h>
#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>

#pragma comment(lib, "iron.core.lib")
#pragma comment(lib, "iron.engine.lib")

using namespace iron;
using namespace iron::window;

int main() {
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

    vtable_windowing* table{ (vtable_windowing*)get_module_vtable(engine_api::windowing, sizeof(vtable_windowing)) };

    factory_windowing factory{ table };
    factory.set_default_background({ 0.1f, 0.2f, 0.4f });
    factory.set_default_titlebar({ 0.2f, 0.2f, 0.2f });
    factory.set_icon("D:\\code\\IronEngine\\iron.ico", { 32, 32 });

    window_init_info window_info{};
    window_info.width = 1024;
    window_info.height = 768;
    window_info.title = "Iron Editor";

    ::iron::window::window main_window{};
    factory.create_window(window_info, &main_window);

    while (factory.is_window_open(main_window)) {
        factory.poll_messages(main_window);
    }

    factory.destroy_window(main_window);
    factory.destroy();

    engine_shutdown();

    return 0;
}
