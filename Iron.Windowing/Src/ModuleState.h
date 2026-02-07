#pragma once
#include <Iron.Windowing/Windowing.h>

#include <Windows.h>
#include <dwmapi.h>

namespace iron::window {
class windowing_state_t {
public:
    windowing_state_t() {
        m_default_background = 0x1c1c1c1c;
        m_default_titlebar = 0xffffffff;
        m_refcount = 0;

        m_icon = LoadIcon(0, IDI_APPLICATION);
    }

    ~windowing_state_t() {
        DestroyIcon(m_icon);

        if (m_refcount > 0) {
            LOG_WARNING("Windowing state destroyed but still has objects referencing it!");
        }
    }

    constexpr void add_ref() {
        ++m_refcount;
    }

    constexpr void remove_ref() {
        --m_refcount;
    }

    constexpr void set_background(math::v3 color) {
        color *= 255.f;
        m_default_background = RGB(
            (u8)color.x,
            (u8)color.y,
            (u8)color.z);
    }

    constexpr void set_titlebar(math::v3 color) {
        color *= 255.f;
        m_default_titlebar = RGB(
            (u8)color.x,
            (u8)color.y,
            (u8)color.z);
    }

    void set_icon(const char* path, math::v2 size) {
        DestroyIcon(m_icon);
        LOG_INFO("Set default icon to: %s", path);
        m_icon = (HICON)LoadImageA(0, path, IMAGE_ICON, (int)size.x, (int)size.y, LR_LOADFROMFILE);
    }

    constexpr u32 const get_background_color() const {
        return m_default_background;
    }

    constexpr u32 const get_titlebar_color() const {
        return m_default_titlebar;
    }

    constexpr HICON get_icon() const {
        return m_icon;
    }

private:
    u32             m_default_background;
    u32             m_default_titlebar;
    HICON           m_icon;
    u32             m_refcount;
};
}
