#pragma once
#include <Iron.Engine/Engine.h>

#include <unordered_map>
#include <string>

namespace iron {
class module_manager {
public:
    ~module_manager();

    result::code load_module(const char* path, u64 id, u64 buffer_size);
    void unload_module(u64 id);

    void* const get_vtable(u64 id) const;

private:
    std::unordered_map<u64, engine_module>  m_modules{};
};
}
