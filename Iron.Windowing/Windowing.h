#pragma once
#include <Iron.Core/Core.h>

MAKE_VERSION(windowing, 0, 1, 0)

namespace iron::window {
typedef class window_t* window;

struct window_init_info;
struct vtable_windowing;
class factory_windowing;

struct window_message {
    enum msg : u32 {
        create = 0,
        close,
    };
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

struct vtable_windowing {
    version         api_version;

    result::code(*initialize)();
    void(*shutdown)();

    windowing_config*(*get_config)();

    result::code(*create_window)(const window_init_info&, window*);
    void(*destroy_window)(window);
    void(*poll_messages)(window);
    bool(*is_window_open)(window);
};

class factory_windowing {
public:
    factory_windowing(vtable_windowing* table) {
        m_table = table;
        CHECK_TABLE(initialize);
        CHECK_TABLE(shutdown);
        CHECK_TABLE(get_config);
        CHECK_TABLE(create_window);
        CHECK_TABLE(destroy_window);
        CHECK_TABLE(poll_messages);
    }

    result::code initialize() const {
        if (!(m_table && m_table->initialize)) UNLIKELY{
            INVALID_TABLE(initialize);
            return result::e_incomplete;
        }

        return m_table->initialize();
    }

    void shutdown() const {
        if (!(m_table && m_table->shutdown)) UNLIKELY{
            INVALID_TABLE(shutdown);
            return;
        }

        m_table->shutdown();
    }

    windowing_config* get_config() const {
        if (!(m_table && m_table->get_config)) UNLIKELY{
            INVALID_TABLE(get_config);
            return nullptr;
        }

        return m_table->get_config();
    }

    result::code create_window(const window_init_info& info, window* handle) const {
        if (!(m_table && m_table->create_window)) UNLIKELY{
            INVALID_TABLE(create_window);
            return result::e_incomplete;
        }

        return m_table->create_window(info, handle);
    }

    void destroy_window(window handle) const {
        if (!(m_table && m_table->destroy_window)) UNLIKELY{
            INVALID_TABLE(destroy_window);
            return;
        }

        m_table->destroy_window(handle);
    }

    void poll_messages(window handle) const {
        if (!(m_table && m_table->poll_messages)) UNLIKELY{
            INVALID_TABLE(poll_messages);
            return;
        }

        m_table->poll_messages(handle);
    }

    bool is_window_open(window handle) const {
        if (!(m_table && m_table->is_window_open)) UNLIKELY{
            INVALID_TABLE(is_window_open);
            return false;
        }

        return m_table->is_window_open(handle);
    }

private:
    vtable_windowing*   m_table{};
};
}
