#include <Iron.Input/Input.h>

#include <Windows.h>

namespace iron::input {
void
input_proc(void* hwnd, u32 msg, u64 wparam, u64 lparam) {
}

constexpr static version    g_version{ 0, 0, 1 };
}

extern "C" __declspec(dllexport)
iron::result::code
get_vtable(iron::input::vtable_input* table) {
    if (!table) {
        return iron::result::e_nullptr;
    }

    table->api_version = iron::input::g_version;

    table->initialize = nullptr;
    table->shutdown = nullptr;
    table->input_proc = iron::input::input_proc;
    table->register_callback = nullptr;
    table->unregister_callback = nullptr;

    return iron::result::ok;
}
