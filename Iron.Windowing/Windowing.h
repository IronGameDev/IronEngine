#pragma once
#include <Iron.Core/Core.h>

MAKE_VERSION(windowing, 0, 1, 0)

namespace iron::window {
typedef class window_t* window;
typedef class windowing_state_t* windowing_state;

struct window_init_info;
struct vtable_windowing;
class factory_windowing;

struct window_message {
    enum msg : u32 {
        create = 0,
        close,
    };
};

struct window_init_info {
    u32             width;
    u32             height;
    const char*     title;
    bool            fullscreen;
};

struct vtable_windowing {
    version         api_version;

    result::code(*create_state)(windowing_state*);
    void(*destroy_state)(windowing_state);

    void(*set_default_background)(windowing_state, math::v3);
    void(*set_default_titlebar)(windowing_state, math::v3);
    void(*set_icon)(windowing_state, const char*, math::v2);

    result::code(*create_window)(windowing_state, const window_init_info&, window*);
    void(*destroy_window)(windowing_state, window);
    void(*poll_messages)(window);
    bool(*is_window_open)(window);
};

class factory_windowing {
public:
    factory_windowing()
        : m_table(), m_ms(module_state::invalid),
        m_state() {
    }

    factory_windowing(vtable_windowing* table)
        : m_table(table), m_ms(module_state::invalid) {
        m_table = table;
        bool complete{ true };

        CHECK_TABLE(create_state, complete);
        CHECK_TABLE(destroy_state, complete);
        CHECK_TABLE(set_default_background, complete);
        CHECK_TABLE(set_icon, complete);
        CHECK_TABLE(create_window, complete);
        CHECK_TABLE(destroy_window, complete);
        CHECK_TABLE(poll_messages, complete);

        if (!m_table) {
            return;
        }

        if (!m_table->create_state) {
            INVALID_TABLE(create_state);
            return;
        }

        if (!m_table->destroy_state) {
            INVALID_TABLE(destroy_state);
            return;
        }

        result::code res{ result::ok };
        res = m_table->create_state(&m_state);
        if (result::fail(res)) {
            LOG_RESULT(res);
            return;
        }

        m_ms = complete ? module_state::complete : module_state::incomplete;
    }

    void destroy() {
        if (module_state::is_valid(m_ms)) {
            m_table->destroy_state(m_state);
            m_table = nullptr;
            m_state = nullptr;
        }

        m_ms = module_state::invalid;
    }

    void set_default_background(math::v3 color) const {
        if (!(module_state::is_valid(m_ms) && m_table->set_default_background)) UNLIKELY{
            INVALID_TABLE(set_default_background);
            return;
        }

        m_table->set_default_background(m_state, color);
    }

    void set_default_titlebar(math::v3 color) const {
        if (!(module_state::is_valid(m_ms) && m_table->set_default_titlebar)) UNLIKELY{
            INVALID_TABLE(set_default_titlebar);
            return;
        }

        m_table->set_default_titlebar(m_state, color);
    }

    void set_icon(const char* path, math::v2 size) const {
        if (!(module_state::is_valid(m_ms) && m_table->set_icon)) UNLIKELY{
            INVALID_TABLE(set_icon);
            return;
        }

        m_table->set_icon(m_state, path, size);
    }

    result::code create_window(const window_init_info& info, window* handle) const {
        if (!(module_state::is_valid(m_ms) && m_table->create_window)) UNLIKELY{
            INVALID_TABLE(create_window);
            return result::e_incomplete;
        }

        return m_table->create_window(m_state, info, handle);
    }

    void destroy_window(window handle) const {
        if (!(module_state::is_valid(m_ms) && m_table->destroy_window)) UNLIKELY{
            INVALID_TABLE(destroy_window);
            return;
        }

        m_table->destroy_window(m_state, handle);
    }

    void poll_messages(window handle) const {
        if (!(module_state::is_valid(m_ms) && m_table->poll_messages)) UNLIKELY{
            INVALID_TABLE(poll_messages);
            return;
        }

        m_table->poll_messages(handle);
    }

    bool is_window_open(window handle) const {
        if (!(module_state::is_valid(m_ms) && m_table->is_window_open)) UNLIKELY{
            INVALID_TABLE(is_window_open);
            return false;
        }

        return m_table->is_window_open(handle);
    }

    constexpr module_state::state get_state() const {
        return m_ms;
    }

private:
    vtable_windowing*   m_table{};
    windowing_state     m_state{};
    module_state::state m_ms{};
};
}
