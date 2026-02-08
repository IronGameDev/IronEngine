#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.Engine/Src/Modules/Modules.h>

#include <filesystem>

namespace iron {
class engine_context {
public:
    engine_context();

    module_manager          m_modules{};

    std::filesystem::path   m_engine_dir{};

public:
    void* load_and_get_vtable(const char* dllName, u64 buffer_size);
};

extern engine_context g_context;
}
