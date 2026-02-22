#include <Iron.Engine/Src/Renderer/Renderer.h>

#include <filesystem>

using namespace Iron::RHI;

#define MAX_ADAPTERS 8

namespace Iron {
RenderContext::RenderContext(void* factoryPtr)
    :
    m_Error(Result::ENotInitialized),
    m_Factory((IRHIFactory*)factoryPtr),
    m_Adapter(),
    m_Device(),
    m_Surface() {
    if (!m_Factory) {
        LOG_FATAL("Render Context can not be initialized without a factory!");
        m_Error = Result::ENullptr;
        return;
    }

    if (!m_Factory->GetNative()) {
        LOG_FATAL("RHI Factory failed to initialize!");
        return;
    }

    const u32 adapter_count{ m_Factory->GetAdapterCount() };

    IRHIAdapter* adapters[MAX_ADAPTERS];
    m_Factory->GetAdapters(&adapters[0], MAX_ADAPTERS);

    m_Adapter = adapters[0];

    for (u32 i{ 0 }; i < adapter_count; ++i) {
        if (adapters[i]->GetType() == AdapterType::Discrete) {
            m_Adapter = adapters[i];
            break;
        }
    }

    if (!m_Adapter) {
        LOG_ERROR("No gpu found!");
        m_Error = Result::ENoInterface;
        return;
    }

    DeviceInitInfo device_info{};
    device_info.Backend = RHIBackend::DirectX12;
    device_info.Debug = false;
    device_info.DisableGPUTimeout = false;

    std::filesystem::path ConfigPath{ "D:\\code\\IronEngine\\" };
    ConfigPath.append("settings.ini");
    ConfigFile Config{};
    if (Result::Success(Config.Load(ConfigPath.string().c_str()))) {
        device_info.Backend = atoi(Config.Get("renderer", "force_legacy", "0")) ? RHIBackend::DirectX11 : RHIBackend::DirectX12;
        device_info.Debug = atoi(Config.Get("renderer", "debug_device", "0"));
        device_info.DisableGPUTimeout = atoi(Config.Get("renderer", "disable_gpu_timeout", "0"));
    }

    Result::Code res{ Result::Ok };
    res = m_Factory->CreateDevice(m_Adapter, device_info, &m_Device);
    if (Result::Fail(res)) {
        LOG_ERROR("Failed to create graphics device!");
        return;
    }

    m_Error = res;
}

RenderContext::~RenderContext() {
    SafeRelease(m_Device);
}

Result::Code
RenderContext::InitializeForWindow(Window::IWindow* const window) {
    if (!window) {
        return Result::ENullptr;
    }

    //if(!window->)

    SurfaceInitInfo surf_info{};
    void*               Native;
    u32                 Width;
    u32                 Height;
    RHIFormat::Fmt      Format;
    bool                TripleBuffering;
    bool                AllowTearing;

    return Result::Ok;
}
}
