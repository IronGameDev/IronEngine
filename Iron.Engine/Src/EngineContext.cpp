#include <Iron.Engine/Src/EngineContext.h>

#include <Windows.h>

namespace iron {
namespace {
constexpr static const char* g_module_names[engine_api::count]{
    "Iron.Windowing.dll",
    "Iron.Renderer.dll",
    "Iron.Input.dll",
    "Iron.Audio.dll",
    "Iron.Filesystem.dll"
};

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
    m_log_enable_filename(true),
    m_headless(true),
    m_running(false) {
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

result::code
engine_context::run(const engine_init_info& info, application* const app)
{
    m_init_info = info;
    m_headless = option::get(info.headless);

    if (!app) {
        LOG_FATAL("No application interface!");
        return result::e_nointerface;
    }

    result::code res{ result::ok };

    if (!m_headless) {
        m_windowing = {
            (window::vtable_windowing*)load_and_get_vtable(
                g_module_names[engine_api::windowing],
                sizeof(window::vtable_windowing)
            )
        };

        m_windowing.set_default_background({ 0.1f, 0.2f, 0.4f });
        m_windowing.set_default_titlebar({ 0.2f, 0.2f, 0.2f });
        m_windowing.set_icon("D:\\code\\IronEngine\\iron.ico", { 32, 32 });

        res = app->pre_initialize();
        if (result::fail(res)) {
            app->shutdown();
            return res;
        }

        window::window_init_info window_info{};
        window_info.width = info.window.width;
        window_info.height = info.window.height;
        window_info.title = info.window.title;
        window_info.fullscreen = info.window.fullscreen;

        res = m_windowing.create_window(window_info, &m_window);
        if (result::fail(res)) {
            LOG_FATAL("A window was requested, but could not be made!");
            return res;
        }
    }

    res = app->post_initialize();
    if (result::fail(res)) {
        app->shutdown();
        return res;
    }

    m_running = true;

    while (m_running.load()) {
        app->frame();

        m_windowing.poll_messages(m_window);
        if (!m_windowing.is_window_open(m_window)) {
            m_running = false;
        }
    }

    app->shutdown();

    if (!m_headless) {
        m_windowing.destroy_window(m_window);
        m_windowing.destroy();
    }

    return result::ok;
}

void*
engine_context::load_and_get_vtable(const char* dllName, u64 buffer_size) {
    std::filesystem::path full_path{ m_engine_dir };
    full_path.append(dllName);
    const u64 id{ fnv1a(dllName) };
    const result::code res{ m_modules.load_module(full_path.string().c_str(), id, buffer_size) };
    if (result::fail(res)) {
        LOG_RESULT(res);
        return nullptr;
    }

    return m_modules.get_vtable(id);
}

void* const
engine_context::get_engine_api(engine_api::api api) {
    switch (api)
    {
    case engine_api::windowing:
        return &m_windowing;
    case engine_api::renderer:
        break;
    case engine_api::input:
        break;
    case engine_api::audio:
        break;
    case engine_api::filesystem:
        break;
    default:
        break;
    }

    return nullptr;
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
    if (!m_headless) {
        if (module_state::is_valid(m_windowing.get_state())) m_windowing.destroy();

    }

    for (u32 i{ 0 }; i < engine_api::count; ++i) {
        m_modules.unload_module(fnv1a(g_module_names[i]));
    }

    m_modules.reset();
    m_cmd_args = {};
}
}
