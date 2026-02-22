#include <Iron.RHI/Src/D3D11/BackendDX11.h>

#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace Iron::RHI::D3D11 {
CRHIDevice_DX11::CRHIDevice_DX11(
    IRHIFactory* const factory,
    IRHIAdapter* const adapter,
    const DeviceInitInfo& info)
    :
    m_Factory(factory),
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
    }
    if (info.DisableGPUTimeout) {
        flags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
    }

    const D3D_FEATURE_LEVEL levels[]{
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
    };
    
    ComPtr<ID3D11Device> device{};
    ComPtr<ID3D11DeviceContext> context{};

    HRESULT hr{ S_OK };
    hr = m_D3D11CreateDevice(
        (IDXGIAdapter*)adapter->GetNative(),
        D3D_DRIVER_TYPE_UNKNOWN,
        0,
        0,//flags
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
    SafeRelease(m_Device);

    if (m_D3D11Dll) {
        FreeLibrary(m_D3D11Dll);
    }

    delete this;
}

void* const
CRHIDevice_DX11::GetNative() const {
    return m_Device;
}
}
