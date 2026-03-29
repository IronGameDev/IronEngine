#include <Iron.RHI/RHI.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>
#include <unordered_map>
#include <deque>

namespace Iron::RHI::D3D12 {
constexpr static u64 Alignment64KB{ 1 << 16 };
constexpr static u64 Alignment4MB{ 1 << 22 };

template<typename T>
struct D3D12FeatureTraits {
    static void PreQuery(T&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS1> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS1;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS1&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS2> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS2;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS2&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS3> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS3;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS3&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS4> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS4;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS4&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS5> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS5;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS5&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS6> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS6;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS6&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS7> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS7;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS7&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS8> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS8;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS8&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS9> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS9;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS9&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS10> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS10;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS10&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS11> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS11;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS11&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS12> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS12;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS12&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS13> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS13;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS13&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS14> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS14;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS14&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS15> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS15;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS15&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS16> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS16;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS16&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS17> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS17;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS17&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS18> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS18;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS18&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS19> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS19;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS19&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS20> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS20;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS20&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_D3D12_OPTIONS21> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_D3D12_OPTIONS21;
    static void PreQuery(D3D12_FEATURE_DATA_D3D12_OPTIONS21&) {}
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_FEATURE_LEVELS> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_FEATURE_LEVELS;

    static void PreQuery(D3D12_FEATURE_DATA_FEATURE_LEVELS& data) {
        constexpr static D3D_FEATURE_LEVEL levels[]{
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        data.NumFeatureLevels = _countof(levels);
        data.pFeatureLevelsRequested = &levels[0];
    }
};

template<>
struct D3D12FeatureTraits<D3D12_FEATURE_DATA_SHADER_MODEL> {
    static constexpr D3D12_FEATURE Feature = D3D12_FEATURE_SHADER_MODEL;

    static void PreQuery(D3D12_FEATURE_DATA_SHADER_MODEL& data) {
        data.HighestShaderModel = D3D_SHADER_MODEL_6_9;
    }
};

struct HeapAllocInfo {
    ID3D12Heap*             Heap;
    u64                     Offset;
    u64                     Size;
    u32                     Order;
    u32                     Index;
};

class DX12SlabAllocator {
public:
    static constexpr u32 HeapLog2{ 26 }; // 64MB
    static constexpr u64 HeapSize{ 1ull << HeapLog2 };

    bool Initialize(ID3D12Device14* const device,
        ResourceUsage::Usage heapUsage,
        u32 slabSize);

    void Release();

    HeapAllocInfo Allocate(u32 size);
    void Free(const HeapAllocInfo& alloc);

private:
    ID3D12Heap*     m_Heap{ nullptr };
    u32             m_SlabSize;
    Vector<u32>     m_FreeIndices;
};

//Buffer usage only!
class DX12BuddyAllocator {
public:
    static constexpr u32 HeapLog2{ 26 }; // 64MB
    static constexpr u32 LeafLog2{ 16 }; // 64KB

    static constexpr u64 HeapSize{ 1ull << HeapLog2 };
    static constexpr u64 LeafSize{ 1ull << LeafLog2 };
    static constexpr u32 OrderCount{ HeapLog2 - LeafLog2 + 1 };   // 11

    static constexpr u32 MaxLeaves{ HeapSize / LeafSize };       // 1024

    bool Initialize(ID3D12Device14* const device,
        ResourceUsage::Usage heapUsage);

    void Release();

    HeapAllocInfo Allocate(u64 size);
    void Free(const HeapAllocInfo& alloc);

private:
    bool FindFreeBlock(u32 order, u32& outIndex);
    void ValidateHandle(const HeapAllocInfo& alloc);

    ID3D12Heap*         m_Heap{ nullptr };
    std::vector<u64>    m_Free[OrderCount]{};
    std::vector<u64>    m_Split[OrderCount]{};
};

class DX12DescriptorHeap {
public:
    Result::Code Initialize(ID3D12Device14* const device,
        u32 size,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        bool shader_visible);

    void Release();

    u32 Allocate();
    u32 Allocate(u32 count);

    void Free(u32 index, u64 fence_value);
    void Free(u32 base, u32 count, u64 fence_value);
    void Collect(u64 fence);

    constexpr D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }

    __forceinline D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(u32 index) const {
        return { m_CpuStart.ptr + SIZE_T(index) * m_IncrementSize };
    }

    __forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(u32 index) const {
        return { m_GpuStart.ptr + UINT64(index) * m_IncrementSize };
    }

private:
    D3D12_DESCRIPTOR_HEAP_TYPE  m_Type{};
    ID3D12DescriptorHeap*       m_Heap{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_CpuStart{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_GpuStart{};
    u32                         m_IncrementSize{};
    bool                        m_ShaderVisible{};
    u32                         m_Capacity{};
    Vector<u32>                 m_FreeList{};

    struct PendingFree {
        u64                 FenceValue;
        u32                 Index;
    };

    std::deque<PendingFree>     m_PendingFrees;
};

class DX12Barriers {
public:
    constexpr static u32 MaxBarriers{ 32 };

    void Add(const D3D12_RESOURCE_BARRIER& barrier) {
        if (m_Current >= MaxBarriers)
            return;

        m_Barriers[m_Current] = barrier;
        ++m_Current;
    }

    void Apply(ID3D12GraphicsCommandList10* const cmd) {
        if (m_Current == 0)
            return;

        cmd->ResourceBarrier(m_Current, &m_Barriers[0]);

        Reset();
    }

    void Reset() {
        m_Current = 0;
    }

private:
    D3D12_RESOURCE_BARRIER  m_Barriers[MaxBarriers]{};
    u32                     m_Current{};
};

class CRHIDevice_DX12 : public IRHIDevice {
public:
    struct ResourceSlot {
        u32         Generation : Id::GenerationBits{};
        u32         DenseIndex : (32 - Id::GenerationBits) {};
    };

    struct DenseResource {
        u32                 SparseIndex{};
        HeapAllocInfo       Info{};
        ID3D12Resource*     Resource{};
    };

    struct PipelineLayout {
        ID3D12RootSignature*    RootSig{};
        u32                     Generation{};
        u32                     NumParams : 16{};
        u32                     NumConstants : 16{};
    };

    struct Pipeline {
        ID3D12PipelineState*    Pso{};
        u32                     Generation{};
        u32                     RefCount{};
        u64                     Hash{};
    };

public:
    CRHIDevice_DX12(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX12() = default;

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

    void* const GetNative() const override;

    inline ID3D12CommandQueue* const GetGraphicsQueue() const {
        return m_GraphicsQueue;
    }

    DX12DescriptorHeap& GetSrvHeap() {
        return m_SrvHeap;
    }

    template<typename T>
    T QueryFeatureSupport() {
        if (!m_Device)
            return T();

        T data{};
        D3D12FeatureTraits<T>::PreQuery(data);

        HRESULT hr{ m_Device->CheckFeatureSupport(
            D3D12FeatureTraits<T>::Feature, &data,
            sizeof(T)) };
        if (FAILED(hr))
            return T();

        return data;
    }

    ID3D12Resource* const ResolveResource(RHIResource handle) const;
    PipelineLayout ResolveLayout(RHIPipelineLayout layout) const;
    ID3D12PipelineState* const ResolvePso(RHIPipeline pso) const;

private:
    D3D_FEATURE_LEVEL _GetMaximumFL(IUnknown* const adapter) const;

    IRHIFactory*                                    m_Factory;
    bool                                            m_Debug;

    HMODULE                                         m_D3D12Dll;
    PFN_D3D12_CREATE_DEVICE                         m_D3D12CreateDevice;
    PFN_D3D12_GET_DEBUG_INTERFACE                   m_D3D12GetDebugInterface;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE    m_SerializeRootSig;
    D3D_FEATURE_LEVEL                               m_FeatureLevel;
    DeviceFeatures                                  m_Features;

    ID3D12Device14*                                 m_Device;
    ID3D12CommandQueue*                             m_GraphicsQueue;

    DX12BuddyAllocator                              m_BufferHeaps;
    DX12DescriptorHeap                              m_SrvHeap;

    Vector<ResourceSlot>                            m_SparseResources;
    Vector<DenseResource>                           m_DenseResources;
    Vector<u32>                                     m_FreeSparseResources;

    Vector<PipelineLayout>                          m_PipelineLayouts;
    Vector<u32>                                     m_FreeLayouts;

    Vector<Pipeline>                                m_Pipelines;
    Vector<u32>                                     m_FreePsos;
    std::unordered_map<u64, u32>                    m_PsoMap;
};

class CRHISurface_DX12 : public IRHISurface {
public:
    constexpr static u32 MaxBuffers{ 3 };

    CRHISurface_DX12(
        CRHIDevice_DX12* const device,
        IRHIFactory* const factory,
        const SurfaceInitInfo& info);

    void Release() override;

    u32 GetBufferCount() const override;
    void* const GetNative() const override;
    void* const GetNativeBuffer(u32 index) const override;

    void Present(bool vsync = true);

    u32 GetCurrentBBIndex() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetBufferDescriptor(u32 index);

private:
    void Finalize(ID3D12Device14* const d3d12);

    IDXGISwapChain4*            m_Swapchain;
    ID3D12Resource*             m_Buffers[MaxBuffers];
    u32                         m_BufferCount;
    bool                        m_SupportTearing;
    ID3D12DescriptorHeap*       m_RtvHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor;
    u32                         m_DescriptorSize;
    u32                         m_CurrentBB;
};

class CRHIFrameGraph_DX12 : public IRHIFrameGraph {
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

    struct BarrierDesc {
        u32                     Resource;
        D3D12_RESOURCE_STATES   Before;
        D3D12_RESOURCE_STATES   After;
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
        Vector<BarrierDesc>     Barriers{};

        FGPassFunc              Func{};

        constexpr inline void EnableClearTarget(u16 slot) {
            ClearedBinds |= (1 << (slot + 1));
        }

        constexpr inline void EnableClearDepth() {
            ClearedBinds |= 0x1;
        }

        constexpr inline bool HasTargetClear(u16 slot) const {
            return ClearedBinds & (1 << (slot + 1));
        }

        constexpr inline bool HasDepthClear() const {
            return ClearedBinds & 0x1;
        }
    };

    struct CommandQueue {
        constexpr static u32 BufferCount{ 3 };

        struct Frame {
            ID3D12CommandAllocator*     Alloc;
            u64                         FenceValue;

            void Wait(ID3D12Fence* const fence, HANDLE fenceEvent) {
                if (fence->GetCompletedValue() < FenceValue) {
                    fence->SetEventOnCompletion(FenceValue, fenceEvent);
                    WaitForSingleObject(fenceEvent, INFINITE);
                }
            }
        };

        Result::Code Initialize(
            ID3D12Device14* const device,
            ID3D12CommandQueue* const queue,
            D3D12_COMMAND_LIST_TYPE type) {
            if (!(device && queue)) {
                return Result::EInvalidarg;
            }

            Queue = queue;

            HRESULT hr{ S_OK };
            for (u32 i{ 0 }; i < BufferCount; ++i) {
                hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&Frames[i].Alloc));
                if (FAILED(hr)) {
                    return Result::ECreateRHIObject;
                }
            }

            hr = device->CreateCommandList(0, type, Frames[0].Alloc, nullptr, IID_PPV_ARGS(&List));
            if (FAILED(hr)) {
                return Result::ECreateRHIObject;
            }

            hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
            if (FAILED(hr)) {
                return Result::ECreateRHIObject;
            }

            FenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
            if (FAILED(hr)) {
                return Result::ECreateResource;
            }

            List->Close();

            return Result::Ok;
        }

        void Release() {
            Flush();

            SafeRelease(List);

            for (u32 i{ 0 }; i < BufferCount; ++i) {
                SafeRelease(Frames[i].Alloc);
            }

            SafeRelease(Fence);
            CloseHandle(FenceEvent);
        }

        void Begin() {
            Frame& frame{ Frames[FrameIndex] };
            frame.Wait(Fence, FenceEvent);
            frame.Alloc->Reset();
            List->Reset(frame.Alloc, nullptr);
        }

        void End(CRHISurface_DX12* const surface) {
            List->Close();
            ID3D12CommandList* const lists[]{ List };
            Queue->ExecuteCommandLists(_countof(lists), &lists[0]);

            const u64 fence_value{ ++FenceValue };
            Frame& frame{ Frames[FrameIndex] };
            frame.FenceValue = fence_value;

            Queue->Signal(Fence, fence_value);
            
            surface->Present();

            FrameIndex = (FrameIndex + 1) % BufferCount;
        }

        void Flush() {
            for (u32 i{ 0 }; i < BufferCount; ++i) {
                Frames[i].Wait(Fence, FenceEvent);
            }

            FrameIndex = 0;
        }

        ID3D12CommandQueue*             Queue{};
        ID3D12GraphicsCommandList10*    List{};
        ID3D12Fence*                    Fence{};
        u64                             FenceValue{};
        HANDLE                          FenceEvent{};
        Frame                           Frames[BufferCount]{};
        u32                             FrameIndex{};
    };

    struct MemoryHeap {
        Result::Code Initialize(
            ID3D12Device14* const device,
            u64 size,
            u64 alignment) {
            if (!device) {
                return Result::ENullptr;
            }

            Size = size;

            D3D12_HEAP_DESC desc{};
            desc.SizeInBytes = size;
            desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            desc.Properties.CreationNodeMask = 0;
            desc.Properties.VisibleNodeMask = 0;
            desc.Alignment = alignment;
            desc.Flags = D3D12_HEAP_FLAG_NONE;

            HRESULT hr{ S_OK };
            hr = device->CreateHeap(&desc, IID_PPV_ARGS(&Heap));
            if (FAILED(hr)) {
                return Result::ECreateRHIObject;
            }

            return Result::Ok;
        }

        void Release() {
            SafeRelease(Heap);
            Size = 0;
        }

        ID3D12Heap*                     Heap{};
        u64                             Size{};
    };

public:
    ~CRHIFrameGraph_DX12() = default;

    Result::Code Initialize(
        CRHIDevice_DX12* const device,
        u32 flags,
        const RHIGraphBuilder& builder);

    void Release() override;

    void Execute(
        IRHISurface* const surface,
        u64 frameNumber) override;

    void WaitIdle() override;

private:
    Vector<Vector<u32>> GetDependencies(const RHIGraphBuilder& builder) const;
    Vector<u32> TopologicalSort(const Vector<Vector<u32>>& dependencies) const;
    void PrintDependencies(const Vector<Vector<u32>>& dependencies) const;

    inline u32 CalculateTemporal(u64 frameNumber, u32 start, u32 count) {
        return start + (u32)(frameNumber % (u64)count);
    }

private:
    CRHIDevice_DX12*                    m_Parent{};

    Vector<Pass>                        m_Passes{};
    Vector<Resource>                    m_DescResources{};
    Vector<View>                        m_DescViews{};
    Vector<ID3D12Resource*>             m_Resources{};

    CommandQueue                        m_Queue{};
    MemoryHeap                          m_Heap{};
    DX12DescriptorHeap                  m_RtvHeap{};
    DX12DescriptorHeap                  m_DsvHeap{};

    struct {
        D3D12_CPU_DESCRIPTOR_HANDLE     CpuStart{};
        D3D12_GPU_DESCRIPTOR_HANDLE     GpuStart{};
        u32                             Start{};
        u32                             Count{};
    } m_SrvHeapData{};
};
}
