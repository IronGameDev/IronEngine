#include <Iron.Engine/Engine.h>

#include <Windows.h>
#include "Modules.h"

namespace iron {
typedef result::code(*func_get_vtable)(void*, u64);

module_manager::~module_manager() {
    reset();
}

result::code
module_manager::load_module(const char* path, u64 id, u64 buffer_size) {
    auto pair{ m_modules.find(id) };
    if (pair != m_modules.end()) {
        return result::ok;
    }

    engine_module mod{};
    mod.id = id;
    mod.library = LoadLibraryExA(path, 0, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!mod.library) {
        return result::e_loadlibrary;
    }

    func_get_vtable func{ (func_get_vtable)GetProcAddress((HMODULE)mod.library, "get_vtable") };
    if (!func) {
        FreeLibrary((HMODULE)mod.library);
        return result::e_getvtable;
    }

    mod.vtable = mem_alloc(buffer_size);

    result::code res{ result::ok };
    res = func(mod.vtable, buffer_size);
    if (result::fail(res)) {
        mem_free(mod.vtable);
        return res;
    }

    LOG_INFO("Loaded module %s Id=%ull", path, id);
    m_modules[id] = mod;

    return res;
}

void
module_manager::unload_module(u64 id) {
    auto pair{ m_modules.find(id) };
    if (pair != m_modules.end() && engine_module::is_loaded(pair->second)) {
        FreeLibrary((HMODULE)pair->second.library);
        mem_free(pair->second.vtable);
        m_modules[id] = {};
    }
}

void* const
module_manager::get_vtable(u64 id) const {
    auto pair{ m_modules.find(id) };
    if (pair != m_modules.end()) {
        return pair->second.vtable;
    }

    return nullptr;
}

void
module_manager::reset() {
    for (auto& pair : m_modules) {
        if (engine_module::is_loaded(pair.second)) {
            LOG_WARNING("Dll Id=%ull was not unloaded!", pair.second.id);
            unload_module(pair.first);
        }
    }

    m_modules = {};
}
}
