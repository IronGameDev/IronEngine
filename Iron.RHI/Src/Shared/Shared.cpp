#include <Iron.RHI/Renderer.h>
#include <Iron.RHI/Src/D3D11/BackendDX11.h>
#include <Iron.RHI/Src/D3D12/BackendDX12.h>

#include <dxgi1_6.h>

#include <vector>
#include <string>
#include <codecvt>
#include <filesystem>

namespace Iron::RHI {
namespace {
using PFN_CREATE_DXGI_FACTORY_2 = HRESULT(*)(UINT, const IID&, void**);

#pragma warning(push)
#pragma warning(disable : 4996)//TODO: Properly handle wchar conversion

inline void
WideCharToUtf8(wchar_t* wide, char* utf8, u32 length) {
    std::wstring ws(wide);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string s = converter.to_bytes(ws);

    strncpy(utf8, s.c_str(), length);
    if (length) utf8[length - 1] = '\0';
}

#pragma warning(pop)

class CRHIAdapter;

class CRHIFactory : public IRHIFactory {
public:
    CRHIFactory();

    void Release() override;

    u32 GetAdapterCount() const override;

    void GetAdapters(
        IRHIAdapter** adapters,
        u32 count) const override;

    Result::Code CreateDevice(
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info,
        IRHIDevice** outHandle) override;

    bool SupportsTearing() const override;
    bool IsDebug() const override;

    void* const GetNative() const override;

private:
    HMODULE                     m_DxgiDll;
    PFN_CREATE_DXGI_FACTORY_2   m_CreateFactory;
    IDXGIFactory7*              m_Factory;
    std::vector<CRHIAdapter>    m_Adapters;
    u32                         m_SupportsTearing;
    bool                        m_EnableDebug;
};

class CRHIAdapter : public IRHIAdapter {
public:
    explicit CRHIAdapter(IDXGIAdapter4* const adapter);

    void Release() override;

    const char* GetName() const override;
    AdapterType::Type GetType() const override;
    u32 GetVendorID() const override;
    u32 GetDeviceID() const override;
    void* const GetNative() const override;

private:
    IDXGIAdapter4*      m_Adapter;
    char                m_Name[RHI_MAX_NAME];
    AdapterType::Type   m_Type;
    u32                 m_VendorID;
    u32                 m_DeviceID;
};

CRHIFactory::CRHIFactory()
    : m_DxgiDll(),
    m_CreateFactory(),
    m_Factory(),
    m_Adapters(),
    m_SupportsTearing(),
    m_EnableDebug() {
    m_DxgiDll = LoadLibraryExA("dxgi.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!m_DxgiDll) {
        LOG_FATAL("Failed to load dxgi.dll!");
        return;
    }

    m_CreateFactory = (PFN_CREATE_DXGI_FACTORY_2)GetProcAddress(m_DxgiDll, "CreateDXGIFactory2");
    if (!m_CreateFactory) {
        LOG_FATAL("Failed to get CreateDXGIFactory2() from dxgi.dll!");
        return;
    }

    std::filesystem::path ConfigPath{ "D:\\code\\IronEngine\\" };
    ConfigPath.append("settings.ini");
    ConfigFile Config{};
    if (Result::Success(Config.Load(ConfigPath.string().c_str()))) {
        m_EnableDebug = atoi(Config.Get("rhi", "debug_factory", "0"));
    }

    u32 flags{ 0 };
    if (m_EnableDebug) {
        flags |= DXGI_CREATE_FACTORY_DEBUG;
        LOG_DEBUG("Enabled debug factory");
    }

    HRESULT hr{ S_OK };
    hr = m_CreateFactory(0, IID_PPV_ARGS(&m_Factory));
    if (FAILED(hr)) {
        LOG_FATAL("Failed to create dxgi factory!");
        return;
    }

    IDXGIAdapter4* adapter{ nullptr };
    for (u32 i{ 0 };
        m_Factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
        ++i) {
        m_Adapters.emplace_back(adapter);
    }

    m_Factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &m_SupportsTearing, sizeof(u32));
}

void
CRHIFactory::Release() {
    for (auto& adapter : m_Adapters) {
        adapter.Release();
    }

    SafeRelease(m_Factory);
    if (m_DxgiDll) {
        FreeLibrary(m_DxgiDll);
    }

    delete this;
}

u32
CRHIFactory::GetAdapterCount() const {
    return (u32)m_Adapters.size();
}

void
CRHIFactory::GetAdapters(IRHIAdapter** adapters,
    u32 count) const {
    if (!adapters) {
        return;
    }

    const u32 num{ Math::Min((u32)m_Adapters.size(), count) };

    for (u32 i{ 0 }; i < num; ++i) {
        adapters[i] = (IRHIAdapter*)&m_Adapters[i];
    }
}

Result::Code
CRHIFactory::CreateDevice(IRHIAdapter* const adapter,
    const DeviceInitInfo& info,
    IRHIDevice** outHandle) {
    if (!(adapter && outHandle)) {
        return Result::ENullptr;
    }

    if (!adapter->GetNative()) {
        return Result::EInvalidarg;
    }

    switch (info.Backend)
    {
    case RHIBackend::DirectX11: {
        D3D11::CRHIDevice_DX11* temp{ nullptr };
        //D3D11::CRHIDevice_DX11* temp{ new D3D11::CRHIDevice_DX11(adapter, info) };
        if (!temp) {
            return Result::ENomemory;
        }

        if (!temp->GetNative()) {
            SafeRelease(temp);
            return Result::ECreateRHIObject;
        }

        *outHandle = temp;
    } break;
    case RHIBackend::DirectX12: {
        D3D12::CRHIDevice_DX12* temp{ new D3D12::CRHIDevice_DX12(this, adapter, info) };
        if (!temp) {
            return Result::ENomemory;
        }

        if (!temp->GetNative()) {
            SafeRelease(temp);
            return Result::ECreateRHIObject;
        }

        *outHandle = temp;
    } break;
    default:
        return Result::EInvalidarg;
    }

    return Result::Ok;
}

bool
CRHIFactory::SupportsTearing() const {
    return m_SupportsTearing;
}

bool
CRHIFactory::IsDebug() const {
    return m_EnableDebug;
}

void* const
CRHIFactory::GetNative() const {
    return m_Factory;
}

CRHIAdapter::CRHIAdapter(IDXGIAdapter4* const adapter)
    : m_Adapter(adapter),
    m_Name(),
    m_Type(),
    m_VendorID(),
    m_DeviceID() {
    if (!m_Adapter) {
        return;
    }

    DXGI_ADAPTER_DESC3 desc;
    m_Adapter->GetDesc3(&desc);

    WideCharToUtf8(&desc.Description[0], &m_Name[0], RHI_MAX_NAME);

    if (desc.DedicatedVideoMemory > 0) {
        m_Type = AdapterType::Discrete;
    }
    else if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
        m_Type = AdapterType::Virtual;
    }
    else {
        m_Type = AdapterType::Integrated;
    }

    m_VendorID = desc.VendorId;
    m_DeviceID = desc.DeviceId;

    constexpr static const char* adapterTypes[]{
        "Unknown",
        "Discrete",
        "Integrated",
        "Virtual",
    };

    LOG_INFO("Adapter info:");
    LOG_INFO("    Name=%s", &m_Name[0]);
    LOG_INFO("    Type=%s", adapterTypes[m_Type]);
    LOG_INFO("    VendorID=%u", TPM_VERTICAL);
    LOG_INFO("    DeviceID=%u", m_DeviceID);
}

void
CRHIAdapter::Release() {
    SafeRelease(m_Adapter);
}

const char*
CRHIAdapter::GetName() const {
    return &m_Name[0];
}

AdapterType::Type
CRHIAdapter::GetType() const {
    return m_Type;
}

u32
CRHIAdapter::GetVendorID() const {
    return m_VendorID;
}

u32
CRHIAdapter::GetDeviceID() const {
    return m_DeviceID;
}

void* const
CRHIAdapter::GetNative() const {
    return m_Adapter;
}
}//anonymous namespace
}

extern "C" __declspec(dllexport)
Iron::Result::Code
GetFactory(Iron::RHI::IRHIFactory** factory) {
    if (!factory) {
        return Iron::Result::ENullptr;
    }

    using namespace Iron::RHI;
    CRHIFactory* temp{ new CRHIFactory() };
    if (!temp) {
        return Iron::Result::ENomemory;
    }

    *factory = temp;

    return Iron::Result::Ok;
}
