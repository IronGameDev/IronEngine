#include <Iron.Windowing/Src/Window.h>
#include <Iron.Windowing/Src/Config.h>

namespace iron {
namespace {
window_t*
get_from_handle(HWND hwnd) {
    return (window_t*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
}

LRESULT CALLBACK
wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg)
    {
    case WM_NCCREATE: {
        LPCREATESTRUCTA create{ (LPCREATESTRUCTA)lparam };
        window_t* temp{ (window_t*)create->lpCreateParams };
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)temp);
    } break;
    default:
        break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
}//anonymous namespace

window_t::window_t(const window_init_info& init_info)
    : m_fullscreen(init_info.fullscreen) {

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = 0;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(
        g_config.default_background.r,
        g_config.default_background.g,
        g_config.default_background.b
    ));
    wc.lpszMenuName = "";
    wc.lpszClassName = "ironwndclass";
    wc.hIconSm = LoadIcon(0, IDI_APPLICATION);

    RegisterClassExA(&wc);

    RECT size{ 0, 0, (LONG)init_info.width, (LONG)init_info.height };
    AdjustWindowRectEx(&size, default_style, FALSE, default_ex_style);

    m_hwnd = CreateWindowExA(default_ex_style,
        wc.lpszClassName,
        init_info.title,
        default_style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (int)(size.right - size.left),
        (int)(size.bottom - size.top),
        0, 0, 0, this);

    if (!m_hwnd) {
        m_error = result::e_createwindow;
        return;
    }

    ShowWindow(m_hwnd, m_fullscreen ? SW_MAXIMIZE : SW_SHOW);
    UpdateWindow(m_hwnd);

    m_error = result::ok;
}

window_t::~window_t() {
    DestroyWindow(m_hwnd);
}

void
window_t::pump_messages() {
    MSG msg{};
    while (PeekMessageA(&msg, m_hwnd, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
}
