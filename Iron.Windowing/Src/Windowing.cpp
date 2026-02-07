#include <Iron.Windowing/Windowing.h>
#include <Iron.Windowing/Src/Window.h>
#include <Iron.Windowing/Src/ModuleState.h>

namespace iron::window {
namespace {
result::code
create_state(windowing_state* state) {
    windowing_state_t* ref{ new windowing_state_t() };
    if (!ref) {
        return result::e_nomemory;
    }

    *state = ref;

    return result::ok;
}

void
destroy_state(windowing_state state) {
    windowing_state_t* ref{ state };
    if (!ref) {
        return;
    }

    delete ref;
}

void
set_default_background(windowing_state state, math::v3 color) {
    windowing_state_t* ref{ state };
    if (!ref) {
        return;
    }

    ref->set_background(color);
}

void
set_default_titlebar(windowing_state state, math::v3 color) {
    windowing_state_t* ref{ state };
    if (!ref) {
        return;
    }

    ref->set_titlebar(color);
}

void
set_icon(windowing_state state, const char* path, math::v2 size) {
    windowing_state_t* ref{ state };
    if (!ref) {
        return;
    }

    ref->set_icon(path, size);
}

result::code
create_window(windowing_state state, const window_init_info& init_info, window* out_handle) {
    windowing_state_t* state_ref{ state };

    if (!state_ref) {
        return result::e_nullptr;
    }

    window_t* ref{ new window_t(state_ref, init_info) };
    if (!ref) {
        return result::e_nomemory;
    }

    *out_handle = ref;
    state_ref->add_ref();

    return result::ok;
}

void
destroy_window(windowing_state state, window handle) {
    window_t* ref{ handle };
    windowing_state_t* state_ref{ state };

    if (!(ref && state_ref)) {
        return;
    }

    state_ref->remove_ref();
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

constexpr static version    g_version{ 0, 1, 0 };
}//anonymous namespace
}

extern "C" __declspec(dllexport)
iron::result::code
get_vtable(iron::window::vtable_windowing* table) {
    if (!table) {
        return iron::result::e_nullptr;
    }

    table->api_version = iron::window::g_version;
    
    table->create_state = iron::window::create_state;
    table->destroy_state = iron::window::destroy_state;
    table->set_default_background = iron::window::set_default_background;
    table->set_default_titlebar = iron::window::set_default_titlebar;
    table->set_icon = iron::window::set_icon;

    table->create_window = iron::window::create_window;
    table->destroy_window = iron::window::destroy_window;
    table->poll_messages = iron::window::poll_messages;
    table->is_window_open = iron::window::is_window_open;

    return iron::result::ok;
}
