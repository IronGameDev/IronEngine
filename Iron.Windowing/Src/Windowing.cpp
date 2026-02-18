#include <Iron.Windowing/Windowing.h>

#include <Windows.h>
#include <dwmapi.h>

namespace Iron::Window {
namespace {
class CWindowFactory;

struct WindowHint {
    enum Hint : u32 {
        None = 0,
        Close,
        Resize,
    };
};

class CWindow final : public IWindow {
public:
    explicit CWindow(
        const WindowInitInfo& initInfo);

    ~CWindow() = default;

    void Release() override;

    void Hint(
        WindowHint::Hint hint);
    void PumpMessages() override;

    void SetBackground(
        Math::V3 color) override;

    void SetBorderColor(
        Math::V3 color) override;

    void SetTitleColor(
        Math::V3 color) override;

    Result::Code SetIcon(
        const char* path,
        Math::V2 size) override;

    void SetTitle(
        const char* title) override;

    void SetFullscreen(
        bool fullscreen) override;

    bool IsOpen() const override {
        return m_Open;
    }

    bool IsFullscreen() const override {
        return m_Fullscreen;
    }

    constexpr DWORD GetBackgroundDWORD() const {
        return m_Background;
    }

private:
    HWND                    m_Hwnd;
    bool                    m_Open;
    bool                    m_Fullscreen;
    DWORD                   m_Background;
    RECT                    m_NormalRect;

    constexpr static DWORD  DefaultStyle{ WS_OVERLAPPEDWINDOW | WS_VISIBLE };
    constexpr static DWORD  DefaultExStyle{ WS_EX_OVERLAPPEDWINDOW };
};

class CWindowFactory final : public IWindowFactory {
public:
    ~CWindowFactory() = default;

    Result::Code OpenWindow(
        const WindowInitInfo& info,
        IWindow** outHandle) override;

    Version GetVersion() const override;

    void Release();

private:
};

CWindow*
GetFromHandle(HWND hwnd) {
    return (CWindow*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg)
    {
    case WM_CLOSE:
    case WM_DESTROY: {
        CWindow* const parent{ GetFromHandle(hwnd) };
        if (!parent) UNLIKELY{
            LOG_ERROR("Close message send to invalid window!");
            break;
        }

        parent->Hint(WindowHint::Close);
    } break;
    case WM_ERASEBKGND: {
        CWindow* const parent{ GetFromHandle(hwnd) };
        if (!parent) UNLIKELY{
            LOG_ERROR("Close message send to invalid window!");
            break;
        }
        
        const HDC hdc{ (HDC)wparam };
        RECT rc{};
        GetClientRect(hwnd, &rc);
        
        const HBRUSH brush{ CreateSolidBrush(parent->GetBackgroundDWORD()) };
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
    } return 1;
    case WM_NCCREATE: {
        LPCREATESTRUCTA create{ (LPCREATESTRUCTA)lparam };
        CWindow* const temp{ (CWindow*)create->lpCreateParams };
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)temp);
    } break;
    default:
        break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

CWindow::CWindow(const WindowInitInfo& initInfo)
    : m_Fullscreen(initInfo.Fullscreen), m_Open(false),
    m_Background(RGB(24, 48, 87)),
    m_NormalRect() {

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = 0;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(m_Background);
    wc.lpszMenuName = "";
    wc.lpszClassName = "ironwndclass";
    wc.hIconSm = LoadIcon(0, IDI_APPLICATION);

    RegisterClassExA(&wc);

    RECT size{ 0, 0, (LONG)initInfo.Width, (LONG)initInfo.Height };
    AdjustWindowRectEx(&size, DefaultStyle, FALSE, DefaultExStyle);

    m_Hwnd = CreateWindowExA(DefaultExStyle,
        wc.lpszClassName,
        initInfo.Title,
        DefaultStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (int)(size.right - size.left),
        (int)(size.bottom - size.top),
        0, 0, 0, this);

    if (!m_Hwnd) {
        return;
    }

    ShowWindow(m_Hwnd, m_Fullscreen ? SW_MAXIMIZE : SW_SHOW);
    UpdateWindow(m_Hwnd);
    GetWindowRect(m_Hwnd, &m_NormalRect);

    m_Open = true;
}

void
CWindow::Release() {
    DestroyWindow(m_Hwnd);
    delete this;
}

void
CWindow::Hint(WindowHint::Hint hint) {
    switch (hint)
    {
    case WindowHint::None: {
        LOG_WARNING("CWindow::Hint was called with a None value, is this the intent?");
    } break;
    case WindowHint::Close: {
        m_Open = false;
    } break;
    case WindowHint::Resize: {
        if (!m_Fullscreen) {
            GetWindowRect(m_Hwnd, &m_NormalRect);
        }
    } break;
    default:
        break;
    }
}

void
CWindow::PumpMessages() {
    MSG msg{};
    while (PeekMessageA(&msg, m_Hwnd, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void
CWindow::SetBackground(Math::V3 color)
{
    color *= 255.f;
    m_Background = RGB(
        (u8)color.X,
        (u8)color.Y,
        (u8)color.Z);

    InvalidateRect(m_Hwnd, NULL, TRUE);
}

void
CWindow::SetBorderColor(Math::V3 color)
{
    color *= 255.f;
    const DWORD col{ RGB(
        (u8)color.X,
        (u8)color.Y,
        (u8)color.Z) };

    DwmSetWindowAttribute(m_Hwnd, DWMWA_BORDER_COLOR, &col, sizeof(col));
    DwmSetWindowAttribute(m_Hwnd, DWMWA_CAPTION_COLOR, &col, sizeof(col));
}

void
CWindow::SetTitleColor(Math::V3 color)
{
    color *= 255.f;
    const DWORD col{ RGB(
        (u8)color.X,
        (u8)color.Y,
        (u8)color.Z) };

    DwmSetWindowAttribute(m_Hwnd, DWMWA_TEXT_COLOR, &col, sizeof(col));
}

Result::Code
CWindow::SetIcon(const char* path,
    Math::V2 size)
{
    const HICON icon{ (HICON)LoadImageA(
        NULL,
        path,
        IMAGE_ICON,
        (int)size.X, (int)size.Y,
        LR_LOADFROMFILE
    ) };

    if (!icon) {
        return Result::ELoadIcon;
    }

    SendMessageA(m_Hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
    SendMessageA(m_Hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);

    return Result::Ok;
}

void
CWindow::SetTitle(const char* title)
{
    if (!title)
        return;

    SetWindowTextA(m_Hwnd, title);
}

void
CWindow::SetFullscreen(bool fullscreen)
{
    if (fullscreen) {
        if (!m_Fullscreen) {
            //Going to fullscreen
            GetWindowRect(m_Hwnd, &m_NormalRect);
            SetWindowLongPtrA(m_Hwnd, GWL_STYLE, 0);
            SetWindowLongPtrA(m_Hwnd, GWL_EXSTYLE, 0);
            ShowWindow(m_Hwnd, SW_SHOWMAXIMIZED);
        }
    }
    else {
        if (m_Fullscreen) {
            //Return to normal
            SetWindowLongPtrA(m_Hwnd, GWL_STYLE, DefaultStyle);
            SetWindowLongPtrA(m_Hwnd, GWL_EXSTYLE, DefaultExStyle);
            ShowWindow(m_Hwnd, SW_SHOWNORMAL);
            MoveWindow(m_Hwnd,
                m_NormalRect.left,
                m_NormalRect.top,
                (m_NormalRect.right - m_NormalRect.left),
                (m_NormalRect.bottom - m_NormalRect.top),
                TRUE);
        }
    }

    m_Fullscreen = fullscreen;
}

Result::Code
CWindowFactory::OpenWindow(const WindowInitInfo& info,
    IWindow** outHandle) {
    CWindow* temp{ new CWindow(info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!temp->IsOpen()) {
        delete temp;
        return Result::ECreatewindow;
    }

    *outHandle = temp;

    return Result::Ok;
}

Version
CWindowFactory::GetVersion() const {
    return Version(0, 1, 0);
}

void
CWindowFactory::Release() {
    delete this;
}
}//anonymous namespace
}

extern "C" __declspec(dllexport)
Iron::Result::Code
GetFactory(Iron::Window::IWindowFactory** factory) {
    if (!factory) {
        return Iron::Result::ENullptr;
    }

    using namespace Iron::Window;
    CWindowFactory* temp{ new CWindowFactory() };
    if (!temp) {
        return Iron::Result::ENomemory;
    }

    *factory = temp;

    return Iron::Result::Ok;
}
