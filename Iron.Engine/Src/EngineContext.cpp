#include <Iron.Engine/Src/EngineContext.h>

#include <Windows.h>

namespace iron {
namespace {
std::filesystem::path
get_exe_path()
{
    char path[MAX_PATH];
    const u32 length{ GetModuleFileNameA(0, &path[0], MAX_PATH) };
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};
    return std::filesystem::path(path).remove_filename();
}
}//anonymous nameapce

engine_context g_context{};

engine_context::engine_context()
{
    enable_log_include_path(option::enable);
    enable_log_level(log_level::debug, option::enable);
    enable_log_level(log_level::info, option::enable);
    enable_log_level(log_level::warning, option::enable);
    enable_log_level(log_level::error, option::enable);
    enable_log_level(log_level::fatal, option::enable);
    
    m_engine_dir = get_exe_path();
}

void*
engine_context::load_and_get_vtable(const char* dllName, u64 buffer_size)
{
    std::filesystem::path full_path{ m_engine_dir };
    full_path.append(dllName);
    const u64 id{ fnv1a(dllName) };
    const result::code res{ m_modules.load_module(full_path.string().c_str(), id, buffer_size)};
    if (result::fail(res)) {
        LOG_RESULT(res);
        return nullptr;
    }

    return m_modules.get_vtable(id);
}
}
