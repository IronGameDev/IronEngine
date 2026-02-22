#include <Iron.RHI/Renderer.h>

#include <d3d12.h>
#include <dxgi1_6.h>

namespace Iron::RHI::D3D12 {
class CRHIDevice_DX12 : public IRHIDevice {
public:
    CRHIDevice_DX12(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX12() = default;

    void Release() override;

    Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) override;

    void* const GetNative() const override;

    inline ID3D12CommandQueue* const GetGraphicsQueue() const {
        return m_GraphicsQueue;
    }

private:
    D3D_FEATURE_LEVEL _GetMaximumFL(IUnknown* const adapter) const;

    IRHIFactory*                    m_Factory;
    bool                            m_Debug;

    HMODULE                         m_D3D12Dll;
    PFN_D3D12_CREATE_DEVICE         m_D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE   m_D3D12GetDebugInterface;
    D3D_FEATURE_LEVEL               m_FeatureLevel;

    ID3D12Device14*                 m_Device;
    ID3D12CommandQueue*             m_GraphicsQueue;
};

class CRHISurface_DX12 : public IRHISurface {
public:
    constexpr static u32 MaxBuffers{ 3 };

    CRHISurface_DX12(
        CRHIDevice_DX12* const device,
        IRHIFactory* const factory,
        const SurfaceInitInfo& info);

    u32 GetBufferCount() const override;
    void* const GetNative() const override;

private:
    IDXGISwapChain4*    m_Swapchain;
    ID3D12Resource*     m_Buffers[MaxBuffers];
    u32                 m_BufferCount;
    bool                m_SupportTearing;
};
}
