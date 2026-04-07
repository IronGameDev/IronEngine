#include <Iron.RHI/RHI.h>
#include <Iron.RHI/Src/DXGIShared/Shared.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <unordered_map>

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

template<>
struct D3D11FeatureTraits<D3D11_FEATURE_DATA_THREADING> {
    static constexpr D3D11_FEATURE Feature = D3D11_FEATURE_THREADING;
    static void PreQuery(D3D11_FEATURE_DATA_THREADING&) {}
};

class CRHIDevice_DX11 : public IRHIDevice {
public:
    struct ResourceSlot {
        u32         Generation : Id::GenerationBits{};
        u32         DenseIndex : (32 - Id::GenerationBits) {};
    };

    struct DenseResource {
        enum ResourceType : u32 {
            EBuffer = 0,
            ETexture1D,
            ETexture2D,
            ETexture3D,
        };

        u32             Type : 2;
        u32             SparseIndex : 30;
        ID3D11Resource* Resource;
    };

    struct PipelineLayout {
        u32                     Start{};
        u32                     Count : 16{};
        u32                     PCSize : 16{};
        u32                     Generation{};
    };

    template<typename T>
    struct PsoHandle {
        T*                      Ptr;
        u32                     Index;

        constexpr bool IsValid() const {
            return Ptr != nullptr;
        }
    };

    template<typename T>
    struct RefCountedStorage {
    public:
        PsoHandle<T> Retrieve(u64 hash) {
            auto it{ m_HashMap.find(hash) };
            if (it != m_HashMap.end()) {
                const u32 index{ it->second };
                m_Data[index].RefCount++;
                return { m_Data[index].Ptr, index };
            }

            return { nullptr, 0 };
        }

        PsoHandle<T> AddNewItem(u64 hash, T* ptr) {
            const u32 index{ m_Data.Size() };
            m_HashMap[hash] = index;
            m_Data.EmplaceBack(ptr, hash, 1);
            return {  ptr, index };
        }

        void Release(PsoHandle<T>& handle) {
            if (handle.Index >= m_Data.Size()) {
                return;
            }

            if (!handle.Ptr) {
                return;
            }

            Item& it{ m_Data[handle.Index] };
            if (--it.RefCount > 0) {
                return;
            }

            SafeRelease(it.Ptr);
            m_HashMap.erase(it.Hash);
            it.Ptr = nullptr;
            handle.Ptr = nullptr;
        }

        void Clear() {
            for (auto& ptr : m_Data) {
                SafeRelease(ptr.Ptr);
            }

            m_Data.Clear();
            m_HashMap.clear();
        }

    private:
        struct Item {
            T*                  Ptr;
            u64                 Hash;
            u32                 RefCount;
        };

        Vector<Item>                    m_Data;
        std::unordered_map<u64, u32>    m_HashMap;

    };

    struct ComputePipeline {
        RHIPipelineLayout               Layout;
        PsoHandle<ID3D11ComputeShader>  CS;
    };

    struct GraphicsPipeline {
        RHIPipelineLayout                   Layout;
        PsoHandle<ID3D11VertexShader>       VS;
        PsoHandle<ID3D11PixelShader>        PS;
        PsoHandle<ID3D11DomainShader>       DS;
        PsoHandle<ID3D11HullShader>         HS;
        PsoHandle<ID3D11GeometryShader>     GS;
        PsoHandle<ID3D11InputLayout>        IL;
        PsoHandle<ID3D11BlendState1>        Blend;
        PsoHandle<ID3D11RasterizerState2>   Rasterizer;
        PsoHandle<ID3D11DepthStencilState>  Depth;
        u32                                 SampleMask;
    };

    struct PipelineData {
        u32                             Graphics : 1;
        u32                             Generation : 31;
        u32                             Index;
    };

public:
    CRHIDevice_DX11(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX11() = default;

    void Release() override;

    Result::Code CreateResource(
        const ResourceInitInfo& info,
        RHIResource* resource) override;

    Result::Code CreatePipelineLayout(
        const PipelineLayoutInitInfo& info,
        RHIPipelineLayout* outHandle) override;

    Result::Code CreateComputePipeline(
        const ComputePipelineInitInfo& info,
        RHIPipeline* outHandle) override;

    Result::Code CreateGraphicsPipeline(
        const GraphicsPipelineInitInfo& info,
        RHIPipeline* outHandle) override;

    Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) override;

    Result::Code CreateFrameGraph(
        const RHIGraphBuilder& builder,
        u32 flags,
        IRHIFrameGraph** outHandle) override;

    void DestroyResource(
        RHIResource resource) override;

    void DestroyPipelineLayout(
        RHIPipelineLayout layout) override;

    void DestroyPipeline(
        RHIPipeline pso) override;

    void GetFeatures(
        DeviceFeatures* features) override;

    Result::Code MapResource(
        RHIResource resource,
        u32 subresource,
        MapType::Type type,
        void** data) const override;

    void UnmapResource(
        RHIResource resource,
        u32 subresource) const override;

    u64 CopySubmitList(
        RHICopyCommandList& list) override {
        return 0;
    }

    void CopyWaitFence(
        u64 fenceValue) override {
    }

    void CopySubmitAndWait(
        RHICopyCommandList& list) override {
    }

    void WaitAllCommands() override {}

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

    const DenseResource ResolveResource(RHIResource handle) const;
    const PipelineData ResolvePipelineData(RHIPipeline pso) const;
    const PipelineLayout ResolvePipelineLayout(RHIPipelineLayout layout) const;
    const GraphicsPipeline ResolveGraphicsPipeline(const PipelineData& data) const;
    const ComputePipeline ResolveComputePipeline(const PipelineData& data) const;

    PipelineLayoutParam* const GetParams(u32 start) {
        if (start >= m_PipelineParams.Size())
            return nullptr;

        return &m_PipelineParams[start];
    }

    constexpr bool HasPC() const {
        return m_Features.PushConstants;
    }

private:
    IRHIFactory*                m_Factory;
    bool                        m_Debug;

    HMODULE                     m_D3D11Dll;
    PFN_D3D11_CREATE_DEVICE     m_D3D11CreateDevice;
    D3D_FEATURE_LEVEL           m_FeatureLevel;

    ID3D11Device5*              m_Device;
    ID3D11DeviceContext4*       m_Context;

    DeviceFeatures              m_Features;

    Vector<ResourceSlot>        m_SparseResources;
    Vector<DenseResource>       m_DenseResources;
    Vector<u32>                 m_FreeSparseResources;

    Vector<PipelineLayoutParam> m_PipelineParams;
    Vector<PipelineLayout>      m_PipelineLayouts;
    Vector<u32>                 m_FreeLayouts;

    Vector<ComputePipeline>     m_ComputePipelines;
    Vector<GraphicsPipeline>    m_GraphicsPipelines;
    Vector<PipelineData>        m_Pipelines;
    Vector<u32>                 m_FreePipelines;

    RefCountedStorage<ID3D11ComputeShader>      m_ComputeShaders;
    RefCountedStorage<ID3D11VertexShader>       m_VertexShaders;
    RefCountedStorage<ID3D11PixelShader>        m_PixelShaders;
    RefCountedStorage<ID3D11DomainShader>       m_DomainShaders;
    RefCountedStorage<ID3D11GeometryShader>     m_GeometryShaders;
    RefCountedStorage<ID3D11HullShader>         m_HullShaders;
    RefCountedStorage<ID3D11InputLayout>        m_InputLayouts;
    RefCountedStorage<ID3D11BlendState1>        m_BlendStates;
    RefCountedStorage<ID3D11RasterizerState2>   m_RasterStates;
    RefCountedStorage<ID3D11DepthStencilState>  m_DepthStates;
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
    ID3D11DeviceContext4*               m_Ctx{};
    CRHIDevice_DX11*                    m_Device{};
};
}
