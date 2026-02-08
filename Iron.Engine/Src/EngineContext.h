#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.Engine/Src/Modules/Modules.h>
#include <Iron.Windowing/Windowing.h>

#include <filesystem>

namespace iron {
class engine_context {
public:
    engine_context();
    ~engine_context();

    result::code run(const engine_init_info& info, application* const app);

    module_manager              m_modules{};
    
    std::vector<std::string>    m_cmd_args{};
    engine_init_info            m_init_info{};

    std::filesystem::path       m_engine_dir{};

    window::factory_windowing   m_windowing{};
    iron::window::window        m_window{};

    std::atomic<bool>           m_running{};

public:
    void* load_and_get_vtable(const char* dllName, u64 buffer_size);
    void* const get_engine_api(engine_api::api api);
    void parse_command_args(s32 argc, char** argv);

    void reset();

public:
    //Log config
    bool                    m_log_enable_debug : 1;
    bool                    m_log_enable_info : 1;
    bool                    m_log_enable_warning : 1;
    bool                    m_log_enable_error : 1;
    bool                    m_log_enable_fatal : 1;
    bool                    m_log_enable_filename : 1;

    //Disable windowing and rendering
    bool                    m_headless : 1;
};

extern engine_context g_context;
}
