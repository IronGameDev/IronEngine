#include <Iron.RHI/Renderer.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>

namespace Iron::RHI::D3D11 {
class CRHIDevice_DX11 : public IRHIDevice {
public:
    CRHIDevice_DX11(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX11() = default;

    void Release() override;

    Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) override;

    Result::Code CreateResource(
        const ResourceInitInfo& info,
        IRHIResource** resource) override;

    Result::Code CreateFrameGraph(
        const RHIGraphBuilder& builder,
        u32 flags,
        IRHIFrameGraph** outHandle) override;

    void* const GetNative() const override;

    constexpr ID3D11DeviceContext4* const GetContext() const {
        return m_Context;
    }

private:
    IRHIFactory*            m_Factory;
    bool                    m_Debug;

    HMODULE                 m_D3D11Dll;
    PFN_D3D11_CREATE_DEVICE m_D3D11CreateDevice;
    D3D_FEATURE_LEVEL       m_FeatureLevel;

    ID3D11Device5*          m_Device;
    ID3D11DeviceContext4*   m_Context;
};

class CRHIResource_DX11 : public IRHIResource {
public:
    CRHIResource_DX11(ID3D11Device* const device,
        const ResourceInitInfo& info);

    void Release() override;
    void* const GetNative() const override;

private:
    union {
        ID3D11Buffer*           m_Buffer;
        ID3D11Texture1D*        m_Texture1D;
        ID3D11Texture2D*        m_Texture2D;
        ID3D11Texture3D*        m_Texture3D;
    };
};

class CRHISurface_DX11 : public IRHISurface {
public:
    CRHISurface_DX11(
        CRHIDevice_DX11* const device,
        IRHIFactory* const factory,
        const SurfaceInitInfo& info);

    void Release() override;

    u32 GetBufferCount() const override;
    void* const GetNative() const override;

private:
    IDXGISwapChain4*    m_Swapchain;
    ID3D11Texture2D*    m_Buffer;
    u32                 m_BufferCount;
    bool                m_SupportTearing;
};

class CRHIFrameGraph_DX11 : public IRHIFrameGraph {
    struct CompiledType {
        enum Type : u16 {
            Undefined = 0x00,
            Texture = 0x01,
            Buffer = 0x02,
        };
    };

    struct ViewType {
        enum Type : u16 {
            ShaderResource = 0x00,
            RenderTarget = 1,
            DepthStencil = 2,
            UnorderedAccess = 3,
        };
    };

    struct Resource {
        u16     Start;
        u16     Count : RHI_TEMPORAL_COUNT_BITS;
        u16     Type : 4;
    };

    struct View {
        u16     Resource;
        u16     Type : 4;
        u16     Index : RHI_VIEW_INDEX_BITS;
    };

    struct Pass {
        u16                     NumRtvs : 3{};
        u16                     HasDsv : 1{};
        u16                     ClearedBinds : 4{};

        ID3D11RenderTargetView* Rtvs[RHI_MAX_TARGET_COUNT]{};
        ID3D11DepthStencilView* Dsv{};
        Math::V3                RtvClearValues[RHI_MAX_TARGET_COUNT]{};
        struct {
            f32                 Depth{};
            u8                  Stencil{};
        } DepthClearValue;

        //TODO: Replace with stream and offset
        Vector<u32>             Srvs{};

        FGPassFunc              Func{};
    };

public:
    ~CRHIFrameGraph_DX11() = default;

    Result::Code Initialize(
        CRHIDevice_DX11* const device,
        u32 flags,
        const RHIGraphBuilder& builder);

    void Release() override;

    void Execute() override;

private:
    Vector<Vector<u32>> GetDependencies(const RHIGraphBuilder& builder) const;
    Vector<u32> TopologicalSort(const Vector<Vector<u32>>& dependencies) const;
    void PrintDependencies(const Vector<Vector<u32>>& dependencies) const;

private:
    Vector<Pass>                        m_Passes;
    Vector<Resource>                    m_DescResources;
    Vector<View>                        m_DescViews;
    Vector<ID3D11Texture2D*>            m_Texture2DHeap;
    Vector<ID3D11ShaderResourceView*>   m_SrvHeap;
    Vector<ID3D11RenderTargetView*>     m_RtvHeap;
    Vector<ID3D11DepthStencilView*>     m_DsvHeap;
};
}
