#include <Iron.RHI/Src/D3D12/BackendDX12.h>
#include <Iron.RHI/Src/Shared/Shared.h>

#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace Iron::RHI::D3D12 {
CRHIDevice_DX12::CRHIDevice_DX12(
    IRHIFactory* const factory,
    IRHIAdapter* const adapter,
    const DeviceInitInfo& info)
    :
    m_Factory(factory),
    m_Debug(false),
    m_D3D12Dll(),
    m_D3D12CreateDevice(),
    m_D3D12GetDebugInterface(),
    m_FeatureLevel(),
    m_Device(),
    m_GraphicsQueue() {

    if (!(adapter && m_Factory)) {
        LOG_ERROR("Invalid adapter/factory given to device!");
        return;
    }

    m_D3D12Dll = LoadLibraryExA("d3d12.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!m_D3D12Dll) {
        LOG_FATAL("Failed to load d3d12.dll!");
        return;
    }

    m_D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(m_D3D12Dll, "D3D12CreateDevice");
    m_D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(m_D3D12Dll, "D3D12GetDebugInterface");
    if (!(m_D3D12CreateDevice && m_D3D12GetDebugInterface)) {
        LOG_FATAL("Failed to get D3D12CreateDevice() and/or D3D12GetDebugInterface() from d3d12.dll!");
        return;
    }

    if (info.Debug) {
        ComPtr<ID3D12Debug3> debug{};
        if (SUCCEEDED(m_D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
            LOG_DEBUG("Enabled debug for d3d12 device!");
        }
        m_Debug = true;
    }

    IUnknown* const dxgi_adapter{ (IUnknown* const)adapter->GetNative() };

    m_FeatureLevel = _GetMaximumFL(dxgi_adapter);
    if (!m_FeatureLevel) {
        LOG_FATAL("D3D12 Not supported!");
        return;
    }

    HRESULT hr{ S_OK };
    hr = m_D3D12CreateDevice(
        dxgi_adapter,
        m_FeatureLevel,
        IID_PPV_ARGS(&m_Device));
    if (FAILED(hr)) {
        LOG_FATAL("D3D12 Not supported!");
        return;
    }

    if (m_Debug) {
        ComPtr<ID3D12InfoQueue> info_queue{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&info_queue)))) {
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        }
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = info.DisableGPUTimeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;

    hr = m_Device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_GraphicsQueue));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create graphics command queue!");
        return;
    }
}

void
CRHIDevice_DX12::Release() {
    SafeRelease(m_GraphicsQueue);

    if (m_Debug) {
        {
            ComPtr<ID3D12InfoQueue> info_queue{};
            if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&info_queue)))) {
                info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
                info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
                info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
            }
        }

        ComPtr<ID3D12DebugDevice> debug_device{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&debug_device)))) {
            debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        }
    }

    SafeRelease(m_Device);
}

Result::Code
CRHIDevice_DX12::CreateSurface(const SurfaceInitInfo& info,
    IRHISurface** surface)
{
    return Result::Ok;
}

void* const
CRHIDevice_DX12::GetNative() const {
    return m_Device;
}

D3D_FEATURE_LEVEL
CRHIDevice_DX12::_GetMaximumFL(IUnknown* const adapter) const
{
    ComPtr<ID3D12Device> device{};
    if (FAILED(m_D3D12CreateDevice(adapter,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device))))
        return (D3D_FEATURE_LEVEL)0;

    const D3D_FEATURE_LEVEL levels[]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_2,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS data{};
    data.NumFeatureLevels = _countof(levels);
    data.pFeatureLevelsRequested = &levels[0];

    device->CheckFeatureSupport(
        D3D12_FEATURE_FEATURE_LEVELS,
        &data,
        sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));

    return data.MaxSupportedFeatureLevel;
}

CRHISurface_DX12::CRHISurface_DX12(
    CRHIDevice_DX12* const device,
    IRHIFactory* const factory,
    const SurfaceInitInfo& info)
    : m_Swapchain(),
    m_Buffers(),
    m_BufferCount(info.TripleBuffering ? 3 : 2),
    m_SupportTearing() {
    if (!(device && factory &&
        info.Native && device->GetGraphicsQueue()
        && factory->GetNative())) {
        return;
    }

    const HWND hwnd{ (const HWND)info.Native };
    IDXGIFactory7* const dxgi{ (IDXGIFactory7* const)factory->GetNative() };
    m_SupportTearing = factory->SupportsTearing();

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
        device->GetGraphicsQueue(),
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

    for (u32 i{ 0 }; i < m_BufferCount; ++i) {
        m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Buffers[i]));
        if (!m_Buffers[i]) {
            LOG_ERROR("Failed to get swapchain buffers!");
            return;
        }
    }
}

u32
CRHISurface_DX12::GetBufferCount() const {
    return m_BufferCount;
}

void* const
CRHISurface_DX12::GetNative() const {
    return m_Swapchain;
}
}
