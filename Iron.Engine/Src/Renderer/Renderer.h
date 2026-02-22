#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.RHI/Renderer.h>
#include <Iron.Windowing/Windowing.h>

namespace Iron {
class RenderContext {
public:
    RenderContext()
        : m_Error(Result::ENotInitialized),
        m_Factory(),
        m_Adapter(),
        m_Device(),
        m_Surface() {
    }

    RenderContext(void* factoryPtr);
    ~RenderContext();

    Result::Code InitializeForWindow(Window::IWindow* const window);

    constexpr Result::Code GetLastResult() const {
        return m_Error;
    }

    constexpr RHI::IRHIFactory* const GetFactory() const {
        return m_Factory;
    }

private:
    Result::Code            m_Error;

    RHI::IRHIFactory*       m_Factory;
    RHI::IRHIAdapter*       m_Adapter;
    RHI::IRHIDevice*        m_Device;

    RHI::IRHISurface*       m_Surface;
};
}
