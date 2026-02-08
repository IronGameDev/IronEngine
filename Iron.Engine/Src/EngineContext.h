#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.Engine/Src/Modules/Modules.h>

#include <filesystem>

namespace iron {
class engine_context {
public:
    engine_context();
    ~engine_context();

    module_manager              m_modules{};
    std::vector<std::string>    m_cmd_args{};

    std::filesystem::path       m_engine_dir{};

public:
    void* load_and_get_vtable(const char* dllName, u64 buffer_size);
    void parse_command_args(s32 argc, char** argv);

    void reset();

private:
    bool                    m_log_enable_debug : 1;
    bool                    m_log_enable_info : 1;
    bool                    m_log_enable_warning : 1;
    bool                    m_log_enable_error : 1;
    bool                    m_log_enable_fatal : 1;
    bool                    m_log_enable_filename : 1;
};

extern engine_context g_context;
}
