#pragma once
#include <Iron.Core/Core.h>

MAKE_VERSION(Windowing, 0, 1, 0)

namespace Iron::Window {
struct WindowInitInfo {
    u32             Width;
    u32             Height;
    const char*     Title;
};

class IWindow;

class IWindowFactory : public IObjectBase {
public:
    virtual ~IWindowFactory() = default;

    virtual Result::Code OpenWindow(
        const WindowInitInfo& info,
        IWindow** outHandle) = 0;

    virtual Version GetVersion() const = 0;
};

class IWindow : public IObjectBase {
public:
    virtual ~IWindow() = default;

    virtual void PumpMessages() = 0;

    virtual void SetBackground(
        Math::V3 color) = 0;
    
    virtual void SetBorderColor(
        Math::V3 color) = 0;

    virtual void SetTitleColor(
        Math::V3 color) = 0;
    
    virtual Result::Code SetIcon(
        const char* path,
        Math::V2 size) = 0;

    virtual void SetTitle(
        const char* title) = 0;

    virtual void SetFullscreen(
        bool fullscreen) = 0;

    virtual bool IsOpen() const = 0;
    virtual bool IsFullscreen() const = 0;
};
}
