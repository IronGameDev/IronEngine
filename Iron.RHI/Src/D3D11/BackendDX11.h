#include <Iron.RHI/RHI.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>

namespace Iron::RHI::D3D11 {
template<typename T>
struct D3D11FeatureTraits {
    static void PreQuery(T&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS1> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS1;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS1&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS2> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS2;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS2&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS3> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS3;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS3&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS4> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS4;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS4&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS5> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS5;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS5&) {}
};

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_D3D11_OPTIONS6> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_D3D11_OPTIONS6;
    static void PreQuery(D3D11_FEATURE_DATA_D3D11_OPTIONS6&) {}
};

class CRHIDevice_DX11 : public IRHIDevice {
public:
    CRHIDevice_DX11(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX11() = default;

    void Release() override;

    Result::Code CreateResource(
        const ResourceInitInfo& info,
        IRHIResource** resource) override;

    Result::Code CreatePipelineLayout(
        const PipelineLayoutInitInfo& info,
        IRHIPipelineLayout** outHandle) override {
        return Result::Ok;
    }

    Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) override;

    Result::Code CreateFrameGraph(
        const RHIGraphBuilder& builder,
        u32 flags,
        IRHIFrameGraph** outHandle) override;

    void GetFeatures(
        DeviceFeatures* features) override;

    void* const GetNative() const override;

    constexpr ID3D11DeviceContext4* const GetContext() const {
        return m_Context;
    }

    template<typename T>
    T QueryFeatureSupport() {
        if (!m_Device)
            return T();

        T data{};
        D3D11FeatureTraits<T>::PreQuery(data);

        HRESULT hr{ m_Device->CheckFeatureSupport(
            D3D11FeatureTraits<T>::Feature, &data,
            sizeof(T)) };
        if (FAILED(hr))
            return T();

        return data;
    }

private:
    IRHIFactory*            m_Factory;
    bool                    m_Debug;

    HMODULE                 m_D3D11Dll;
    PFN_D3D11_CREATE_DEVICE m_D3D11CreateDevice;
    D3D_FEATURE_LEVEL       m_FeatureLevel;

    ID3D11Device5*          m_Device;
    ID3D11DeviceContext4*   m_Context;

    DeviceFeatures          m_Features;
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

    void Present(bool vsync = true);

    u32 GetBufferCount() const override;
    void* const GetNative() const override;
    void* const GetNativeBuffer(u32 index) const override;

    ID3D11RenderTargetView1* const GetRtv() const {
        return m_Rtv;
    }

private:
    IDXGISwapChain4*            m_Swapchain;
    ID3D11Texture2D*            m_Buffer;
    ID3D11RenderTargetView1*    m_Rtv;
    u32                         m_BufferCount;
    bool                        m_SupportTearing;
};

class CRHIFrameGraph_DX11 : public IRHIFrameGraph {
    struct CompiledType {
        enum Type : u16 {
            Undefined = 0,
            Texture,
            Buffer,
            Swapchain,
        };
    };

    struct ViewType {
        enum Type : u16 {
            ShaderResource = 0,
            RenderTarget,
            DepthStencil,
            UnorderedAccess,
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
        u16     BaseIndex : RHI_VIEW_INDEX_BITS;
    };

    struct BoundView {
        u16     View;
        u16     Slot;
    };

    struct Pass {
        u16                     NumRtvs : 3{};
        u16                     HasDsv : 1{};
        u16                     ClearedBinds : (RHI_MAX_TARGET_COUNT + 1) {};

        BoundView               Rtvs[RHI_MAX_TARGET_COUNT]{};
        BoundView               Dsv{};
        Math::V4                RtvClearValues[RHI_MAX_TARGET_COUNT]{};
        struct {
            f32                 Depth{};
            u8                  Stencil{};
        } DepthClearValue;

        //TODO: Replace with stream and offset
        Vector<BoundView>       Srvs{};

        FGPassFunc              Func{};

        constexpr inline void EnableClearTarget(u16 slot) {
            ClearedBinds |= (1 << (slot + 1));
        }

        constexpr inline void EnableClearDepth() {
            ClearedBinds |= 0x1;
        }

        constexpr inline bool HasTargetClear(u16 slot) {
            return ClearedBinds & (1 << (slot + 1));
        }

        constexpr inline bool HasDepthClear() {
            return ClearedBinds & 0x1;
        }
    };

public:
    ~CRHIFrameGraph_DX11() = default;

    Result::Code Initialize(
        CRHIDevice_DX11* const device,
        u32 flags,
        const RHIGraphBuilder& builder);

    void Release() override;

    void Execute(
        IRHISurface* const surface,
        u64 frameNumber) override;

    void WaitIdle() override {}

private:
    Vector<Vector<u32>> GetDependencies(const RHIGraphBuilder& builder) const;
    Vector<u32> TopologicalSort(const Vector<Vector<u32>>& dependencies) const;
    void PrintDependencies(const Vector<Vector<u32>>& dependencies) const;

    inline u32 CalculateTemporal(u64 frameNumber, u32 start, u32 count) {
        return start + (u32)(frameNumber % (u64)count);
    }

private:
    Vector<Pass>                        m_Passes;
    Vector<Resource>                    m_DescResources;
    Vector<View>                        m_DescViews;
    Vector<ID3D11Texture2D*>            m_Texture2DHeap;
    Vector<ID3D11ShaderResourceView1*>  m_SrvHeap;
    Vector<ID3D11RenderTargetView1*>    m_RtvHeap;
    Vector<ID3D11DepthStencilView*>     m_DsvHeap;
    
    //TODO: FIX TS
    ID3D11DeviceContext4*               m_Ctx;
};
}
