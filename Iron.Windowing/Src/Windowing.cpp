#include <Iron.Windowing/Windowing.h>
#include <Iron.Windowing/Src/Config.h>
#include <Iron.Windowing/Src/Window.h>

namespace iron {
namespace {
result::code
initialize() {
    g_config.default_background.r = 24;
    g_config.default_background.g = 48;
    g_config.default_background.b = 84;

    return result::ok;
}

windowing_config*
get_config() {
    return &g_config;
}

result::code
create_window(const window_init_info& init_info, window* out_handle) {
    window_t* ref{ new window_t(init_info) };
    if (!ref) {
        return result::e_nomemory;
    }

    *out_handle = ref;

    return result::ok;
}

void
destroy_window(window handle) {
    window_t* ref{ handle };
    if (!ref) {
        return;
    }

    delete ref;
}

void
poll_messages(window handle) {
    window_t* ref{ handle };
    if (!ref) {
        return;
    }

    ref->pump_messages();
}

bool
is_window_open(window handle) {
    window_t* ref{ handle };
    if (!ref) {
        return false;
    }

    return ref->is_open();
}

constexpr static version    g_version{ 0, 0, 1 };
}//anonymous namespace

windowing_config            g_config{};
}

extern "C" __declspec(dllexport)
iron::result::code
get_vtable(iron::vtable_windowing* table) {
    if (!table) {
        return iron::result::e_nullptr;
    }

    table->api_version = iron::g_version;
    
    table->initialize = iron::initialize;
    table->shutdown = nullptr;
    table->get_config = iron::get_config;

    table->create_window = iron::create_window;
    table->destroy_window = iron::destroy_window;
    table->poll_messages = iron::poll_messages;
    table->is_window_open = iron::is_window_open;

    return iron::result::ok;
}
