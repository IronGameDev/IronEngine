#include <Iron.RHI/Src/D3D11/BackendDX11.h>
#include <Iron.RHI/Src/Shared/Shared.h>

#include <wrl.h>
#include <unordered_map>
#include <algorithm>
#include <queue>

using Microsoft::WRL::ComPtr;

namespace Iron::RHI::D3D11 {
namespace {
static const GUID WKPDID_D3DDebugObjectName =
{ 0x429b8c22, 0x9188, 0x4b0c, { 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00 } };

constexpr D3D11_USAGE
ConvertUsage(ResourceUsage::Usage usage) {
    switch (usage)
    {
    case ResourceUsage::Default:
        return D3D11_USAGE_DEFAULT;
    case ResourceUsage::Immutable:
        return D3D11_USAGE_IMMUTABLE;
    case ResourceUsage::Dynamic:
        return D3D11_USAGE_DYNAMIC;
    case ResourceUsage::Copy:
        return D3D11_USAGE_STAGING;
    default:
        return D3D11_USAGE_DEFAULT;
    }
}

constexpr u32
ConvertResourceBindFlags(ResourceFlags::Flags flags) {
    u32 value{ 0 };
    if (flags & ResourceFlags::AllowShaderResource) value |= D3D11_BIND_SHADER_RESOURCE;
    if (flags & ResourceFlags::BindVertexBuffer) value |= D3D11_BIND_VERTEX_BUFFER;
    if (flags & ResourceFlags::BindIndexBuffer) value |= D3D11_BIND_INDEX_BUFFER;

    return value;
}

constexpr u32
ConvertResourceMiscFlags(ResourceFlags::Flags flags) {
    u32 value{ 0 };
    if (flags & ResourceFlags::TextureCube) value |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (flags & ResourceFlags::Structured) value |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    return value;
}

inline void SetDebugName(ID3D11DeviceChild* object, const char* name) {
#if defined(_DEBUG)
    if (object) {
        object->SetPrivateData(
            WKPDID_D3DDebugObjectName,
            (UINT)strlen(name),
            name
        );
    }
#endif
}
}//anonymous namespace

CRHIDevice_DX11::CRHIDevice_DX11(
    IRHIFactory* const factory,
    IRHIAdapter* const adapter,
    const DeviceInitInfo& info)
    :
    m_Factory(factory),
    m_Debug(false),
    m_D3D11Dll(),
    m_D3D11CreateDevice(),
    m_FeatureLevel(),
    m_Device(),
    m_Context() {
    if (!(adapter && m_Factory)) {
        LOG_ERROR("Invalid adapter/factory given to device!");
        return;
    }

    m_D3D11Dll = LoadLibraryExA("d3d11.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!m_D3D11Dll) {
        LOG_FATAL("Failed to load d3d11.dll!");
        return;
    }

    m_D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(m_D3D11Dll, "D3D11CreateDevice");
    if (!m_D3D11CreateDevice) {
        LOG_FATAL("Failed to get D3D11CreateDevice() from d3d11.dll!");
        return;
    }

    UINT flags{ 0 };
    if (info.Debug) {
        flags |= D3D11_CREATE_DEVICE_DEBUG;
        LOG_DEBUG("Enabled debug for d3d11 device!");
        m_Debug = true;
    }
    if (info.DisableGPUTimeout) {
        flags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
    }

    const D3D_FEATURE_LEVEL levels[]{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    ComPtr<ID3D11Device> device{};
    ComPtr<ID3D11DeviceContext> context{};

    HRESULT hr{ S_OK };
    hr = m_D3D11CreateDevice(
        (IDXGIAdapter*)adapter->GetNative(),
        D3D_DRIVER_TYPE_UNKNOWN,
        0,
        flags,
        &levels[0],
        _countof(levels),
        D3D11_SDK_VERSION,
        &device,
        &m_FeatureLevel,
        &context);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create d3d11 device!");
        return;
    }

    device->QueryInterface(IID_PPV_ARGS(&m_Device));
    context->QueryInterface(IID_PPV_ARGS(&m_Context));
}

void
CRHIDevice_DX11::Release() {
    SafeRelease(m_Context);

    if (m_Debug) {
        ComPtr<ID3D11Debug> debug{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
        }
    }

    SafeRelease(m_Device);

    if (m_D3D11Dll) {
        FreeLibrary(m_D3D11Dll);
    }

    delete this;
}

Result::Code
CRHIDevice_DX11::CreateResource(const ResourceInitInfo& info,
    IRHIResource** resource) {
    CRHIResource_DX11* temp{ new CRHIResource_DX11(m_Device, info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!temp->GetNative()) {
        delete temp;
        return Result::ECreateResource;
    }

    *resource = temp;

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreateSurface(const SurfaceInitInfo& info,
    IRHISurface** surface)
{
    CRHISurface_DX11* temp{ new CRHISurface_DX11(this, m_Factory, info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!temp->GetNative()) {
        delete temp;
        return Result::ECreateRHIObject;
    }

    *surface = temp;

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreateFrameGraph(const RHIGraphBuilder& builder,
    u32 flags,
    IRHIFrameGraph** outHandle)
{
    CRHIFrameGraph_DX11* temp{ new CRHIFrameGraph_DX11() };
    if (!temp) {
        return Result::ENomemory;
    }

    Result::Code res{ Result::Ok };
    res = temp->Initialize(this, flags, builder);

    if (Result::Fail(res)) {
        delete temp;
        return res;
    }

    *outHandle = temp;

    return Result::Ok;
}

void* const
CRHIDevice_DX11::GetNative() const {
    return m_Device;
}

CRHIResource_DX11::CRHIResource_DX11(ID3D11Device* const device,
    const ResourceInitInfo& info)
    : m_Buffer() {
    if (!device) {
        return;
    }

    u32 cpu_flags{ 0 };
    if (info.CPURead) cpu_flags |= D3D11_CPU_ACCESS_READ;
    if (info.CPUWrite) cpu_flags |= D3D11_CPU_ACCESS_WRITE;

    const DXGI_FORMAT format{ Shared::ConvertFormat(info.Format) };
    const D3D11_USAGE usage{ ConvertUsage(info.Usage) };
    const u32 bind_flags{ ConvertResourceBindFlags((ResourceFlags::Flags)info.Flags) };
    const u32 misc_flags{ ConvertResourceMiscFlags((ResourceFlags::Flags)info.Flags) };

    switch (info.Dimension)
    {
    case ResourceDimension::Buffer: {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = info.Width;
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;
        desc.StructureByteStride = info.StructuredStride;

        device->CreateBuffer(&desc, nullptr, &m_Buffer);
    } break;
    case ResourceDimension::Texture1D: {
        D3D11_TEXTURE1D_DESC desc{};
        desc.Width = info.Width;
        desc.MipLevels = info.MipLevels;
        desc.ArraySize = info.DepthOrArray;
        desc.Format = format;
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;

        device->CreateTexture1D(&desc, nullptr, &m_Texture1D);
    } break;
    case ResourceDimension::Texture2D: {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = info.Width;
        desc.Height = info.Height;
        desc.MipLevels = info.MipLevels;
        desc.ArraySize = info.DepthOrArray;
        desc.Format = format;
        desc.SampleDesc = { 1, 0 };
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;

        device->CreateTexture2D(&desc, nullptr, &m_Texture2D);
    } break;
    case ResourceDimension::Texture3D: {
        D3D11_TEXTURE3D_DESC desc{};
        desc.Width = info.Width;
        desc.Height = info.Height;
        desc.Depth = info.DepthOrArray;
        desc.MipLevels = info.MipLevels;
        desc.Format = format;
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;

        device->CreateTexture3D(&desc, nullptr, &m_Texture3D);
    } break;
    default:
        LOG_WARNING("Invalid resource dimension!");
        break;
    }
}

void
CRHIResource_DX11::Release() {
    SafeRelease(m_Buffer);
    delete this;
}

void* const
CRHIResource_DX11::GetNative() const {
    return m_Buffer;
}

CRHISurface_DX11::CRHISurface_DX11(
    CRHIDevice_DX11* const device,
    IRHIFactory* const factory,
    const SurfaceInitInfo& info)
    : m_Swapchain(),
    m_Buffer(),
    m_Rtv(),
    m_BufferCount(info.TripleBuffering ? 3 : 2),
    m_SupportTearing() {
    if (!(device && factory &&
        info.Native && device->GetContext()
        && factory->GetNative())) {
        return;
    }

    const HWND hwnd{ (const HWND)info.Native };
    IDXGIFactory7* const dxgi{ (IDXGIFactory7* const)factory->GetNative() };
    ID3D11Device5* const d3d11{ (ID3D11Device5* const)device->GetNative() };
    m_SupportTearing = factory->SupportsTearing() && info.AllowTearing;

    DXGI_SWAP_CHAIN_DESC1 swap_desc{};
    swap_desc.Width = info.Width;
    swap_desc.Height = info.Height;
    swap_desc.Format = Shared::ConvertFormat(info.Format);
    swap_desc.Stereo = FALSE;
    swap_desc.SampleDesc = { 1, 0 };
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = m_BufferCount;
    swap_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_desc.Flags = m_SupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> temp{};

    HRESULT hr{ S_OK };
    hr = dxgi->CreateSwapChainForHwnd(
        d3d11,
        hwnd,
        &swap_desc,
        nullptr,
        nullptr,
        &temp);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create swapchain!");
        return;
    }

    temp->QueryInterface(IID_PPV_ARGS(&m_Swapchain));

    m_Swapchain->GetBuffer(0, IID_PPV_ARGS(&m_Buffer));
    d3d11->CreateRenderTargetView1(m_Buffer, nullptr, &m_Rtv);
}

void
CRHISurface_DX11::Release() {
    SafeRelease(m_Rtv);
    SafeRelease(m_Buffer);
    SafeRelease(m_Swapchain);

    delete this;
}

void
CRHISurface_DX11::Present(bool vsync) {
    m_Swapchain->Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
}

u32
CRHISurface_DX11::GetBufferCount() const {
    return m_BufferCount;
}

void* const
CRHISurface_DX11::GetNative() const {
    return m_Swapchain;
}

void* const
CRHISurface_DX11::GetNativeBuffer(u32 index) const {
    UNREFERENCED_PARAMETER(index);
    return m_Buffer;
}

Result::Code
CRHIFrameGraph_DX11::Initialize(
    CRHIDevice_DX11* const device,
    u32 flags,
    const RHIGraphBuilder& builder) {
    if (!device) {
        return Result::ENullptr;
    }

    const Vector<Vector<u32>> dependencies{ GetDependencies(builder) };
    if ((u32)(flags & FGCompileFlags::LogInfo)) {
        PrintDependencies(dependencies);
    }

    m_Ctx = device->GetContext();

    ID3D11Device5* const d3d11{ (ID3D11Device5* const)device->GetNative() };
    const Vector<u32> sorted{ TopologicalSort(dependencies) };
    const Vector<RHIGraphBuilder::FGPassDesc>& passes{ builder.GetPasses() };
    const Vector<FGResourceInitInfo>& resources{ builder.GetResources() };
    const bool debug_names{ (const bool)(flags & FGCompileFlags::DebugNames) };

    m_Passes.Resize(sorted.Size());
    for (u32 i{ 0 }; i < m_Passes.Size(); ++i) {
        m_Passes[i].Func = passes[sorted[i]].Func;
    }

    {//Resource creation
        m_DescResources.Resize(resources.Size());

        u32 num_texture2ds{};
        u32 num_buffers{};

        for (u32 i{ 0 }; i < resources.Size(); ++i) {
            const FGResourceInitInfo& info{ resources[i] };
            if (info.Type == FGResourceType::Texture) {
                Resource res{};
                res.Start = (u16)num_texture2ds;
                res.Count = (u16)info.TemporalCount;
                res.Type = CompiledType::Texture;

                m_DescResources[i] = res;

                num_texture2ds += info.TemporalCount;
            }
            else if (info.Type == FGResourceType::Buffer) {
                Resource res{};
                res.Start = (u16)num_buffers;
                res.Count = (u16)info.TemporalCount;
                res.Type = CompiledType::Buffer;

                m_DescResources[i] = res;

                num_buffers += info.TemporalCount;
            }
            else if (info.Type == FGResourceType::Swapchain) {
                Resource res{};
                res.Start = 0;
                res.Count = (u16)info.TemporalCount;
                res.Type = CompiledType::Swapchain;

                m_DescResources[i] = res;
            }
        }

        m_Texture2DHeap.Resize(num_texture2ds);

        for (u32 i{ 0 }; i < resources.Size(); ++i) {
            const FGResourceInitInfo& info{ resources[i] };
            const u8 info_flags{ info.GetFlags() };
            u32 bind_flags{ 0 };
            if (info_flags & RHIGraphBuilder::ResourceFlag_SRV) {
                bind_flags |= D3D11_BIND_SHADER_RESOURCE;
            }
            if (info_flags & RHIGraphBuilder::ResourceFlag_RTV) {
                bind_flags |= D3D11_BIND_RENDER_TARGET;
            }
            if (info_flags & RHIGraphBuilder::ResourceFlag_DSV) {
                bind_flags |= D3D11_BIND_DEPTH_STENCIL;
            }
            if (info_flags & RHIGraphBuilder::ResourceFlag_UAV) {
                bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            if (info.Type == FGResourceType::Texture) {
                D3D11_TEXTURE2D_DESC tex_desc{};
                tex_desc.Width = info.Width;
                tex_desc.Height = info.Height;
                tex_desc.MipLevels = info.MipLevels;
                tex_desc.ArraySize = info.DepthOrArray;
                tex_desc.Format = Shared::ConvertFormat(info.StorageFormat);
                tex_desc.SampleDesc = { 1, 0 };
                tex_desc.Usage = D3D11_USAGE_DEFAULT;
                tex_desc.BindFlags = bind_flags;
                tex_desc.CPUAccessFlags = 0;
                tex_desc.MiscFlags = 0;
                const Resource& logical{ m_DescResources[i] };

                for (u32 j{ 0 }; j < info.TemporalCount; ++j) {
                    const u32 index{ logical.Start + j };
                    HRESULT hr{ S_OK };
                    hr = d3d11->CreateTexture2D(&tex_desc, nullptr, &m_Texture2DHeap[index]);
                    if (FAILED(hr)) {
                        LOG_ERROR("Failed to create graph resource!");
                        return Result::ECreateResource;
                    }

                    if (debug_names) {
                        SetDebugName(m_Texture2DHeap[index], info.Name);
                    }
                }

                MemFree((void*)info.Name);
            }
        }
    }
    using namespace Shared;
    std::unordered_map<ViewCacheEntry, u32, ViewCacheHasher> view_cache;

    for (u32 i{ 0 }; i < (u32)m_Passes.Size(); ++i) {
        const RHIGraphBuilder::FGPassDesc& pass_desc{ passes[i] };
        Pass& compiled{ m_Passes[i] };

        for (u32 j{ 0 }; j < (u32)pass_desc.Reads.Size(); ++j) {
            const auto& read{ pass_desc.Reads[j] };
            const u32 slot{ read.Slot };
            const Resource& resource{ m_DescResources[read.Resource] };

            if (resource.Type == CompiledType::Swapchain) {
                continue;
            }

            ViewCacheEntry entry{};
            entry.Resource = read.Resource;
            entry.Format = read.ViewFormat;
            entry.BaseMip = read.Range.BaseMip;
            entry.MipCount = read.Range.MipCount;
            entry.BaseLayer = read.Range.BaseLayer;
            entry.LayerCount = read.Range.LayerCount;
            entry.Plane = read.Range.Plane;

            auto it = view_cache.find(entry);
            if (it == view_cache.end()) {
                if (read.State & ResourceState::PixelResource
                    || read.State & ResourceState::NonPixelResource) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::ShaderResource;
                    view.BaseIndex = (u16)m_SrvHeap.Size();

                    D3D11_SHADER_RESOURCE_VIEW_DESC1 srv_desc{};
                    srv_desc.Format = ConvertFormat(entry.Format);
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MipLevels = entry.MipCount;
                    srv_desc.Texture2D.MostDetailedMip = entry.BaseMip;
                    srv_desc.Texture2D.PlaneSlice = entry.Plane;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11ShaderResourceView1* viewhandle{ nullptr };
                        hr = d3d11->CreateShaderResourceView1(texture, &srv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_SrvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    BoundView binding{};
                    binding.View = (u16)view_id;
                    binding.Slot = (u16)slot;
                    compiled.Srvs.PushBack(binding);
                }
                else if (read.State & ResourceState::DepthRead) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::DepthStencil;
                    view.BaseIndex = (u16)m_DsvHeap.Size();

                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Format = ConvertFormat(entry.Format);
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsv_desc.Texture2D.MipSlice = entry.BaseMip;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11DepthStencilView* viewhandle{ nullptr };
                        hr = d3d11->CreateDepthStencilView(texture, &dsv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_DsvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)view_id;
                    compiled.Dsv.Slot = 0;
                }
            }
            else {
                if (read.State & ResourceState::PixelResource
                    || read.State & ResourceState::NonPixelResource) {
                    BoundView binding{};
                    binding.View = (u16)it->second;
                    binding.Slot = (u16)slot;
                    compiled.Srvs.PushBack(binding);
                }
                else if (read.State & ResourceState::DepthRead) {
                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)it->second;
                    compiled.Dsv.Slot = (u16)slot;
                }
            }
        }

        for (u32 j{ 0 }; j < (u32)pass_desc.Writes.Size(); ++j) {
            const auto& write{ pass_desc.Writes[j] };
            const u32 slot{ write.Slot };
            const Resource& resource{ m_DescResources[write.Resource] };

            if (write.ClearOp == FGClearOp::RenderTarget) {
                compiled.RtvClearValues[slot].X = write.ClearValue.Color[0];
                compiled.RtvClearValues[slot].Y = write.ClearValue.Color[1];
                compiled.RtvClearValues[slot].Z = write.ClearValue.Color[2];
                compiled.RtvClearValues[slot].W = write.ClearValue.Color[3];
                compiled.EnableClearTarget((u16)slot);
            }
            else if (write.ClearOp == FGClearOp::DepthStencil) {
                compiled.DepthClearValue.Depth = write.ClearValue.Depth.Depth;
                compiled.DepthClearValue.Stencil = write.ClearValue.Depth.Stencil;
                compiled.EnableClearDepth();
            }

            if (resource.Type == CompiledType::Swapchain) {

                compiled.NumRtvs++;
                compiled.Rtvs[slot].Slot = (u16)slot;
                compiled.Rtvs[slot].View = (u16)~0;
                continue;
            }

            ViewCacheEntry entry{};
            entry.Resource = write.Resource;
            entry.Format = write.ViewFormat;
            entry.BaseMip = write.Range.BaseMip;
            entry.MipCount = write.Range.MipCount;
            entry.BaseLayer = write.Range.BaseLayer;
            entry.LayerCount = write.Range.LayerCount;
            entry.Plane = write.Range.Plane;
            auto it = view_cache.find(entry);
            if (it == view_cache.end()) {
                if (write.State & ResourceState::RenderTarget) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::RenderTarget;
                    view.BaseIndex = (u16)m_RtvHeap.Size();

                    D3D11_RENDER_TARGET_VIEW_DESC1 rtv_desc{};
                    rtv_desc.Format = ConvertFormat(entry.Format);
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtv_desc.Texture2D.MipSlice = entry.BaseMip;
                    rtv_desc.Texture2D.PlaneSlice = entry.Plane;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11RenderTargetView1* viewhandle{ nullptr };
                        hr = d3d11->CreateRenderTargetView1(texture, &rtv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_RtvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };

                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.NumRtvs++;
                    compiled.Rtvs[slot].Slot = (u16)slot;
                    compiled.Rtvs[slot].View = (u16)view_id;
                }
                else if (write.State & ResourceState::DepthWrite) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::DepthStencil;
                    view.BaseIndex = (u16)m_DsvHeap.Size();

                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Format = ConvertFormat(entry.Format);
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsv_desc.Texture2D.MipSlice = entry.BaseMip;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11DepthStencilView* viewhandle{ nullptr };
                        hr = d3d11->CreateDepthStencilView(texture, &dsv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_DsvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)view_id;
                    compiled.Dsv.Slot = 0;
                }
            }
            else {
                if (write.State & ResourceState::RenderTarget) {
                    compiled.NumRtvs++;
                    compiled.Rtvs[slot].Slot = (u16)slot;
                    compiled.Rtvs[slot].View = (u16)it->second;
                }
                else if (write.State & ResourceState::DepthRead) {
                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)it->second;
                    compiled.Dsv.Slot = (u16)write.Slot;
                }
            }
        }
    }

    return Result::Ok;
}

void
CRHIFrameGraph_DX11::Release() {
    for (auto& res : m_Texture2DHeap) SafeRelease(res);
    for (auto& res : m_SrvHeap) SafeRelease(res);
    for (auto& res : m_RtvHeap) SafeRelease(res);
    for (auto& res : m_DsvHeap) SafeRelease(res);
}

void
CRHIFrameGraph_DX11::Execute(
    IRHISurface* const surface,
    u64 frameNumber) {
    CRHISurface_DX11* const dx_surface{ (CRHISurface_DX11* const)surface };

    ID3D11DeviceContext4* const ctx{ m_Ctx };

    for (auto& pass : m_Passes) {
        const u32 num_rtvs{ pass.NumRtvs };
        const bool has_dsv{ (bool)pass.HasDsv };

        ID3D11RenderTargetView* targets[RHI_MAX_TARGET_COUNT]{};
        ID3D11DepthStencilView* depth{ nullptr };

        for (u16 i{ 0 }; i < num_rtvs; ++i) {
            const u16 view_handle{ pass.Rtvs[i].View };
            if (view_handle == (u16)~0) {
                targets[i] = dx_surface->GetRtv();
            }
            else {
                const View& view_desc{ m_DescViews[view_handle] };
                const Resource& resource{ m_DescResources[view_desc.Resource] };

                const u32 actual_index{ CalculateTemporal(frameNumber, view_desc.BaseIndex, resource.Count) };

                targets[i] = m_RtvHeap[actual_index];
            }

            if (pass.HasTargetClear(i)) {
                ctx->ClearRenderTargetView(targets[i],
                    &pass.RtvClearValues[i].X);
            }
        }

        if (has_dsv) {
            const u16 view_handle{ pass.Dsv.View };
            const View& view_desc{ m_DescViews[view_handle] };
            const Resource& resource{ m_DescResources[view_desc.Resource] };

            const u32 actual_index{ CalculateTemporal(frameNumber, view_desc.BaseIndex, resource.Count) };

            depth = m_DsvHeap[actual_index];
            if (pass.HasDepthClear()) {

                //TODO: Only depth or stencil???
                ctx->ClearDepthStencilView(depth,
                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                    pass.DepthClearValue.Depth,
                    pass.DepthClearValue.Stencil);
            }
        }

        ctx->OMSetRenderTargets(num_rtvs, &targets[0], depth);
    }

    dx_surface->Present();
}

Vector<Vector<u32>>
CRHIFrameGraph_DX11::GetDependencies(
    const RHIGraphBuilder& builder) const {
    const Vector<RHIGraphBuilder::FGPassDesc>& passes{ builder.GetPasses() };
    const u32 num_passes{ passes.Size() };

    Vector<Vector<u32>> dependencies(num_passes);

    std::unordered_map<FGResource, Vector<u32>> writers;
    writers.reserve(num_passes * 2);

    for (u32 passIndex{ 0 }; passIndex < num_passes; ++passIndex) {
        for (const RHIGraphBuilder::FGResourceUsage& write : passes[passIndex].Writes) {
            writers[write.Resource].PushBack(passIndex);
        }
    }

    for (u32 passIndex{ 0 }; passIndex < num_passes; ++passIndex) {
        auto& deps = dependencies[passIndex];

        for (const RHIGraphBuilder::FGResourceUsage& read : passes[passIndex].Reads) {
            auto it = writers.find(read.Resource);
            if (it == writers.end())
                continue;

            for (u32 writerPass : it->second) {
                if (writerPass != passIndex) {
                    deps.PushBack(writerPass);
                }
            }
        }

        // Optional: remove duplicates
        std::sort(deps.begin(), deps.end());
        deps.Erase(std::unique(deps.begin(), deps.end()), deps.end());
    }

    return dependencies;
}

Vector<u32>
CRHIFrameGraph_DX11::TopologicalSort(
    const Vector<Vector<u32>>& dependencies) const {
    const u32 numPasses = static_cast<u32>(dependencies.Size());

    Vector<u32> inDegree(numPasses, 0);
    Vector<std::vector<u32>> dependents(numPasses);

    for (u32 pass = 0; pass < numPasses; ++pass) {
        for (u32 dep : dependencies[pass]) {
            ++inDegree[pass];
            dependents[dep].push_back(pass);
        }
    }

    std::queue<u32> ready;
    for (u32 i = 0; i < numPasses; ++i) {
        if (inDegree[i] == 0) {
            ready.push(i);
        }
    }

    Vector<u32> sorted;
    sorted.Reserve(numPasses);

    while (!ready.empty()) {
        u32 pass = ready.front();
        ready.pop();

        sorted.PushBack(pass);

        for (u32 dependent : dependents[pass]) {
            if (--inDegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (sorted.Size() != numPasses) {
        // Cycle detected: graph is invalid
        LOG_ERROR("D3D12RHI: Cycle detected in Task Graph!");
    }

    return sorted;
}

void
CRHIFrameGraph_DX11::PrintDependencies(
    const Vector<Vector<u32>>& dependencies) const {
    LOG_DEBUG("Task Graph Dependencies:");

    for (u32 pass = 0; pass < dependencies.Size(); ++pass) {
        LOG_DEBUG("Pass %u depends on: ");

        if (dependencies[pass].Empty()) {
            LOG_DEBUG("(none)");
        }
        else {
            for (u32 dep : dependencies[pass]) {
                LOG_DEBUG("    %u", dep);
            }
        }
    }
}
}
