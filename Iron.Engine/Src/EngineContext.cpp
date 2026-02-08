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
    : m_log_enable_debug(true),
    m_log_enable_info(true),
    m_log_enable_warning(true),
    m_log_enable_error(true),
    m_log_enable_fatal(true),
    m_log_enable_filename(true) {
    m_engine_dir = get_exe_path();

    std::filesystem::path config_path{ "D:\\code\\IronEngine\\" };
    config_path.append("settings.ini");
    config_file config{};
    if (result::success(config.load(config_path.string().c_str()))) {
        m_log_enable_debug = atoi(config.get("engine.log", "enable_debug", "1"));
        m_log_enable_info = atoi(config.get("engine.log", "enable_info", "1"));
        m_log_enable_warning = atoi(config.get("engine.log", "enable_warning", "1"));
        m_log_enable_error = atoi(config.get("engine.log", "enable_error", "1"));
        m_log_enable_fatal = atoi(config.get("engine.log", "enable_fatal", "1"));
        m_log_enable_filename = atoi(config.get("engine.log", "enable_filename", "1"));
    }

    enable_log_level(log_level::debug, m_log_enable_debug ? option::enable : option::disable);
    enable_log_level(log_level::info, m_log_enable_info ? option::enable : option::disable);
    enable_log_level(log_level::warning, m_log_enable_warning ? option::enable : option::disable);
    enable_log_level(log_level::error, m_log_enable_error ? option::enable : option::disable);
    enable_log_level(log_level::fatal, m_log_enable_fatal ? option::enable : option::disable);
    enable_log_include_path(m_log_enable_filename ? option::enable : option::disable);
}

engine_context::~engine_context() {
    std::filesystem::path config_path{ "D:\\code\\IronEngine\\" };
    config_path.append("settings.ini");
    config_file config{};
    config.set("engine.log", "enable_debug", std::to_string(m_log_enable_debug).c_str());
    config.set("engine.log", "enable_info", std::to_string(m_log_enable_info).c_str());
    config.set("engine.log", "enable_warning", std::to_string(m_log_enable_warning).c_str());
    config.set("engine.log", "enable_error", std::to_string(m_log_enable_error).c_str());
    config.set("engine.log", "enable_fatal", std::to_string(m_log_enable_fatal).c_str());
    config.set("engine.log", "enable_filename", std::to_string(m_log_enable_filename).c_str());

    if (result::fail(config.save(config_path.string().c_str()))) {
        LOG_ERROR("Failed to save engine config settings!");
    }

    reset();
}

void*
engine_context::load_and_get_vtable(const char* dllName, u64 buffer_size) {
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

void
engine_context::parse_command_args(s32 argc, char** argv) {
    for (s32 i{ 0 }; i < argc; ++i) {
        LOG_INFO("Parsed command argument %s", argv[i]);
        m_cmd_args.emplace_back(argv[i]);
    }
}

void
engine_context::reset() {
    m_modules.reset();
    m_cmd_args = {};
}
}
