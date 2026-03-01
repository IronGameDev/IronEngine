#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.RHI/Renderer.h>
#include <Iron.Windowing/Windowing.h>

namespace Iron {
class RenderContext {
public:
    RenderContext(void* factoryPtr);

    Result::Code InitializeForWindow(Window::IWindow* const window);

    void Release();

    constexpr Result::Code GetLastResult() const {
        return m_Error;
    }

    constexpr RHI::IRHIFactory* const GetFactory() const {
        return m_Factory;
    }

private:
    Result::Code            m_Error{ Result::ENotInitialized };

    RHI::IRHIFactory*       m_Factory{};
    RHI::IRHIAdapter*       m_Adapter{};
    RHI::IRHIDevice*        m_Device{};

    RHI::IRHISurface*       m_Surface{};
    RHI::IRHIFrameGraph*    m_FrameGraph{};

    bool                    m_AllowTearing : 1{};
    bool                    m_TripleBuffering : 1{};
};
}
