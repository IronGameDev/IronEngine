#include <Iron.Engine/Engine.h>

#include <Windows.h>

namespace iron {
typedef result::code(*get_vtable)(void*);

result::code
load_module(const char* path, engine_module* handle) {
    handle->library = LoadLibraryExA(path, 0, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!handle->library) {
        return result::e_loadlibrary;
    }

    get_vtable func{ (get_vtable)GetProcAddress((HMODULE)handle->library, "get_vtable") };
    if (!func) {
        return result::e_getvtable;
    }

    result::code res{ func(handle->vtable) };
    if (res != result::ok) {
        if (res == result::e_incomplete) {
            LOG_WARNING("Module vtable marked as incomplete! Functionality may be limited");
        }
        else {
            return result::e_getvtable;
        }
    }

    return res;
}

void
unload_module(engine_module handle) {
    if (handle.library) {
        FreeLibrary((HMODULE)handle.library);
    }
}
}
