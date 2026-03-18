#include <Iron.Engine/Src/Renderer/Renderer.h>

#include <filesystem>

using namespace Iron::RHI;

#define MAX_ADAPTERS 8

namespace Iron {
namespace {
RHIPipelineLayout   layout{};
RHIPipeline         pso{};

void
RenderStuff(RHICommandBuilder& ctx) {
}

void
PostProcess(RHICommandBuilder& ctx) {
    Viewport vp{ 0.f, 0.f, 1024.f, 768.f, 0.f, 1.f };
    ScissorRect rc{ 0, 0, 1024, 768 };

    ctx.SetGraphicsLayout(layout);
    ctx.SetPipeline(pso);
    ctx.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    ctx.SetViewports(&vp, 1);
    ctx.SetScissors(&rc, 1);

    ctx.Draw(3, 0);
}

void
SetupRenderer(RHIGraphBuilder& builder) {
    //auto depth = builder.CreateResource({
    //    "Depth",
    //    FGResourceType::Texture,
    //    1920, 1080,
    //    RHIFormat::R32_TYPELESS,
    //    });
    //auto color = builder.CreateResource({
    //    "Color",
    //    FGResourceType::Texture,
    //    1920, 1080,
    //    RHIFormat::R8G8B8A8_TYPELESS
    //    });
    //auto ssao = builder.CreateResource({
    //    "SSAO",
    //    FGResourceType::Texture,
    //    1920, 1080,
    //    RHIFormat::R8_TYPELESS
    //    });
    auto bb = builder.RegisterOutput(
        "Swapchain",
        1024,
        768,
        RHIFormat::R8G8B8A8_TYPELESS,
        true
    );

    //builder.BeginPass("DepthPrePass", RenderStuff);
    //builder.AddDepth(depth, RHIFormat::D32_FLOAT, ResourceState::DepthWrite);
    //builder.EndPass();

    //builder.BeginPass("SSAOPass", RenderStuff);
    //builder.Read(depth, RHIFormat::R32_FLOAT, ResourceState::NonPixelResource, 0);
    //builder.Write(ssao, RHIFormat::R8_UNORM, ResourceState::RenderTarget, 0);
    //builder.EndPass();

    //builder.BeginPass("ColorPass", RenderStuff);
    //builder.AddDepth(depth, RHIFormat::D32_FLOAT, ResourceState::DepthRead);
    //builder.Read(ssao, RHIFormat::R8_UNORM, ResourceState::PixelResource, 0);
    //builder.Write(color, RHIFormat::R8G8B8A8_UNORM, ResourceState::RenderTarget, 0);
    //builder.EndPass();

    builder.BeginPass("PostProcess", PostProcess);
    //builder.Read(depth, RHIFormat::R32_FLOAT, ResourceState::PixelResource, 0);
    //builder.Read(color, RHIFormat::R8G8B8A8_UNORM, ResourceState::PixelResource, 1);
    builder.WriteClear(bb, RHIFormat::R8G8B8A8_UNORM, ResourceState::RenderTarget, 0, { 0.f, 0.7f, 0.f, 1.f });
    builder.EndPass();
}
}//anonymous namespace

RenderContext::RenderContext(void* factoryPtr)
    : m_Factory((IRHIFactory*)factoryPtr) {
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

    LOG_INFO("Initializing for %s", m_Adapter->GetName());

    std::filesystem::path ConfigPath{ "D:\\code\\IronEngine\\" };
    ConfigPath.append("settings.ini");
    ConfigFile Config{};
    if (Result::Fail(Config.Load(ConfigPath.string().c_str()))) {
        LOG_WARNING("Failed to load settings.ini for renderer!");
    }

    DeviceInitInfo device_info{};
    device_info.Backend = atoi(Config.Get("renderer", "force_legacy", "0")) ? RHIBackend::DirectX11 : RHIBackend::DirectX12;
    device_info.Debug = atoi(Config.Get("renderer", "debug_device", "0"));
    device_info.DisableGPUTimeout = atoi(Config.Get("renderer", "disable_gpu_timeout", "0"));
    device_info.MaxShaderResources = atoi(Config.Get("renderer", "max_shader_resources", "2048"));

    m_AllowTearing = atoi(Config.Get("renderer", "allow_tearing", "0"));
    m_TripleBuffering = atoi(Config.Get("renderer", "triple_buffer", "1"));

    Result::Code res{ Result::Ok };
    res = m_Factory->CreateDevice(m_Adapter, device_info, &m_Device);
    if (Result::Fail(res)) {
        LOG_ERROR("Failed to create graphics device!");
        return;
    }

    m_Error = res;

    PipelineLayoutParam params[1]{};
    params[0].AsSRV(0, 0);

    PipelineLayoutInitInfo l_info{};
    l_info.NumParams = _countof(params);
    l_info.Params = &params[0];

    m_Device->CreatePipelineLayout(l_info, &layout);

    GraphicsPipelineInitInfo pso_info{ layout };
    u8* vs_blob{};
    u64 vs_size{};
    u8* ps_blob{};
    u64 ps_size{};

    if (device_info.Backend == RHIBackend::DirectX12) {
        ReadFile("D:\\code\\IronEngine\\EngineAssets\\D3D12\\Bin\\FullscreenVS.bin", vs_blob, vs_size);
        ReadFile("D:\\code\\IronEngine\\EngineAssets\\D3D12\\Bin\\ColorPS.bin", ps_blob, ps_size);
    }
    else {
        ReadFile("D:\\code\\IronEngine\\EngineAssets\\D3D11\\Bin\\FullscreenVS.bin", vs_blob, vs_size);
        ReadFile("D:\\code\\IronEngine\\EngineAssets\\D3D11\\Bin\\ColorPS.bin", ps_blob, ps_size);
    }

    pso_info.VS.Blob = vs_blob + sizeof(u32);
    pso_info.VS.Size = vs_size - sizeof(u32);
    pso_info.PS.Blob = ps_blob + sizeof(u32);
    pso_info.PS.Size = ps_size - sizeof(u32);
    pso_info.TargetFormats[0] = RHIFormat::R8G8B8A8_UNORM;
    pso_info.NumTargets = 1;
    pso_info.DepthStencil.DepthEnable = false;
    pso_info.Rasterizer.Cull = CullMode::None;

    m_Device->CreateGraphicsPipeline(pso_info, &pso);

    MemFree(vs_blob);
    MemFree(ps_blob);
}

Result::Code
RenderContext::InitializeForWindow(Window::IWindow* const window) {
    if (!window) {
        return Result::ENullptr;
    }

    if (!window->GetNative()) {
        return Result::EInvalidarg;
    }

    SurfaceInitInfo surf_info{};
    surf_info.Native = window->GetNative();
    surf_info.Width = window->GetWidth();
    surf_info.Height = window->GetHeight();
    surf_info.Format = RHIFormat::R8G8B8A8_UNORM;
    surf_info.TripleBuffering = m_TripleBuffering;
    surf_info.AllowTearing = m_AllowTearing;

    Result::Code res{ Result::Ok };
    res = m_Device->CreateSurface(surf_info, &m_Surface);
    if (Result::Fail(res)) {
        LOG_ERROR("Could not create render surface!");
    }

    ResourceInitInfo info{};
    info.Dimension = ResourceDimension::Buffer;
    info.Width = 1024;
    info.Height = 1;
    info.DepthOrArray = 1;
    info.StructuredStride = 0;
    info.MipLevels = 1;
    info.Format = RHIFormat::UNKNOWN;
    info.Usage = ResourceUsage::Default;
    info.CPURead = false;
    info.CPUWrite = true;
    info.Flags = ResourceFlags::AllowShaderResource;

    RHIResource resss[3]{};

    m_Device->CreateResource(info, &resss[0]);

    m_Device->DestroyResource(resss[0]);

    m_Device->CreateResource(info, &resss[0]);
    m_Device->CreateResource(info, &resss[1]);
    m_Device->CreateResource(info, &resss[2]);
    m_Device->DestroyResource(resss[0]);
    m_Device->DestroyResource(resss[1]);

    m_Device->CreateResource(info, &resss[0]);

    m_Device->DestroyResource(resss[2]);
    m_Device->DestroyResource(resss[0]);

    m_Device->CreateResource(info, &resss[0]);
    m_Device->DestroyResource(resss[0]);

    RHIGraphBuilder builder{};
    SetupRenderer(builder);

    m_Device->CreateFrameGraph(builder, FGCompileFlags::LogInfo | FGCompileFlags::DebugNames, &m_FrameGraph);

    return res;
}

void
RenderContext::Release() {
    m_FrameGraph->WaitIdle();

    m_Device->DestroyPipelineLayout(layout);
    m_Device->DestroyPipeline(pso);

    SafeRelease(m_FrameGraph);
    SafeRelease(m_Surface);
    SafeRelease(m_Device);

    delete this;
}

Result::Code
RenderContext::RenderFrame(u64 frameNumber) {
    m_FrameGraph->Execute(m_Surface, frameNumber);

    return Result::Ok;
}
}
