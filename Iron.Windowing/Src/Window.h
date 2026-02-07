#pragma once
#include <Iron.Windowing/Windowing.h>

#include <Windows.h>

namespace iron::window {
class window_t {
public:
    window_t(const window_init_info& init_info);
    ~window_t();

    void pump_messages();

    void close() {
        m_open = false;
    }

    constexpr result::code get_error() const {
        return m_error;
    }

    constexpr bool is_open() const {
        return m_open;
    }

private:
    HWND            m_hwnd;
    result::code    m_error;
    bool            m_fullscreen;
    bool            m_open;

    constexpr static DWORD default_style{ WS_OVERLAPPEDWINDOW | WS_VISIBLE };
    constexpr static DWORD default_ex_style{ WS_EX_OVERLAPPEDWINDOW };
};
}
