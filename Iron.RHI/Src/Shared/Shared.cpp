#include <Iron.RHI/Renderer.h>

#include <dxgi1_6.h>

namespace Iron::RHI {
using PFN_CREATE_DXGI_FACTORY_2 = HRESULT(*)(UINT, const IID&, void**);

class CRHIFactory : public IRHIFactory {
public:
    CRHIFactory();

    void Release() override;

    u32 GetAdapterCount() const override {
        return 0;
    }
    
    void GetAdapters(
        IRHIAdapter** adapters,
        u32 count) const override {
    }

    void* const GetNative() const override {
        return m_Factory;
    }

private:
    HMODULE                     m_DxgiDll;
    PFN_CREATE_DXGI_FACTORY_2   m_CreateFactory;
    IDXGIFactory7*              m_Factory;
};

CRHIFactory::CRHIFactory()
    : m_DxgiDll(),
    m_CreateFactory(),
    m_Factory() {
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

    HRESULT hr{ S_OK };
    hr = m_CreateFactory(0, IID_PPV_ARGS(&m_Factory));
    if (FAILED(hr)) {
        LOG_FATAL("Failed to create dxgi factory!");
        return;
    }
}

void
CRHIFactory::Release() {
    SafeRelease(m_Factory);
    if (m_DxgiDll) {
        FreeLibrary(m_DxgiDll);
    }

    m_DxgiDll = nullptr;
    m_CreateFactory = nullptr;
    m_Factory = nullptr;
}
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
