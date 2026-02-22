#include <Iron.RHI/Renderer.h>

#include <d3d11_4.h>

namespace Iron::RHI::D3D11 {
class CRHIDevice_DX11 : public IRHIDevice {
public:
    CRHIDevice_DX11(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX11() = default;

    void Release() override;

    void* const GetNative() const override;

private:
    IRHIFactory*            m_Factory;

    HMODULE                 m_D3D11Dll;
    PFN_D3D11_CREATE_DEVICE m_D3D11CreateDevice;
    D3D_FEATURE_LEVEL       m_FeatureLevel;

    ID3D11Device5*          m_Device;
    ID3D11DeviceContext4*   m_Context;
};
}
