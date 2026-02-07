#pragma once
#include <Iron.Core/Core.h>

MAKE_VERSION(input, 0, 2, 0)

namespace iron::input {
using input_callback = void(*)(input_type, key::code, modifier_key, void*);

struct vtable_input {
    version api_version;

    result::code(*initialize)();
    void(*shutdown)();

    void(*input_proc)(void*, u32, u64, u64);
    
    void(*register_callback)(input_callback callback, void* user_data);
    void(*unregister_callback)(input_callback callback);
};

// Factory wrapper
class factory_input {
public:
    factory_input(vtable_input* table) {
        m_table = table;
        CHECK_TABLE(initialize);
        CHECK_TABLE(shutdown);
        CHECK_TABLE(input_proc);
        CHECK_TABLE(register_callback);
        CHECK_TABLE(unregister_callback);
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

    void input_proc(void* hwnd, u32 msg, u64 wparam, u64 lparam) const {
        if (!(m_table && m_table->input_proc)) UNLIKELY{
            INVALID_TABLE(input_proc);
            return;
        }
        m_table->input_proc(hwnd, msg, wparam, lparam);
    }

    void register_callback(input_callback cb, void* user_data) const {
        if (!(m_table && m_table->register_callback)) UNLIKELY{
            INVALID_TABLE(register_callback);
            return;
        }
        m_table->register_callback(cb, user_data);
    }

    void unregister_callback(input_callback cb) const {
        if (!(m_table && m_table->unregister_callback)) UNLIKELY{
            INVALID_TABLE(unregister_callback);
            return;
        }
        m_table->unregister_callback(cb);
    }

private:
    vtable_input* m_table{};
};
} // namespace iron::input
