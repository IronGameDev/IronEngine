#pragma once
#include <Iron.Core/Core.h>

namespace iron {
struct windowing_config;
struct window_init_info;

typedef class window_t* window;

struct vtable_windowing {
    version         api_version;

    result::code(*initialize)();
    void(*shutdown)();

    windowing_config*(*get_config)();

    result::code(*create_window)(const window_init_info&, window*);
    void(*destroy_window)(window);
    void(*poll_messages)(window);
};

struct windowing_config {
    struct {
        u8              r, g, b;
    } default_background;
};

struct window_init_info {
    u32             width;
    u32             height;
    const char*     title;
    bool            fullscreen;
};

class factory_windowing {
public:
    factory_windowing(vtable_windowing table) {
        m_table = table;
        CHECK_TABLE(initialize);
        CHECK_TABLE(shutdown);
        CHECK_TABLE(get_config);
        CHECK_TABLE(create_window);
        CHECK_TABLE(destroy_window);
        CHECK_TABLE(poll_messages);
    }

    result::code initialize() const {
        if (!m_table.initialize) {
            INVALID_TABLE(initialize);
            return result::e_incomplete;
        }        
                
        return m_table.initialize();
    }

    void shutdown() const {
        if (!m_table.shutdown) {
            INVALID_TABLE(shutdown);
            return;
        }

        m_table.shutdown();
    }

    windowing_config* get_config() const {
        if (!m_table.get_config) {
            INVALID_TABLE(get_config);
            return nullptr;
        }

        return m_table.get_config();
    }

private:
    vtable_windowing    m_table{};
};
}
