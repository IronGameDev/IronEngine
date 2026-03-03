#include <Iron.RHI/Renderer.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>
#include <deque>

namespace Iron::RHI::D3D12 {
constexpr static u64 Alignment64KB{ 1 << 16 };
constexpr static u64 Alignment4MB{ 1 << 22 };

struct HeapAllocInfo {
    ID3D12Heap* const       Heap;
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
    CRHIDevice_DX12(
        IRHIFactory* const factory,
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info);
    ~CRHIDevice_DX12() = default;

    void Release() override;

    Result::Code CreateResource(
        const ResourceInitInfo& info,
        IRHIResource** resource) override;

    Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) override;

    Result::Code CreateFrameGraph(
        const RHIGraphBuilder& builder,
        u32 flags,
        IRHIFrameGraph** outHandle) override;

    void* const GetNative() const override;

    inline ID3D12CommandQueue* const GetGraphicsQueue() const {
        return m_GraphicsQueue;
    }

    DX12DescriptorHeap& GetSrvHeap() {
        return m_SrvHeap;
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

    DX12BuddyAllocator              m_BufferHeaps;
    DX12DescriptorHeap              m_SrvHeap;
};

class CRHIResource_DX12 : public IRHIResource {
public:
    CRHIResource_DX12(ID3D12Resource* ptr,
        DX12BuddyAllocator* heap,
        const HeapAllocInfo& alloc)
        : m_Resource(ptr), m_IsBuffer(true), Buffer{ heap, alloc } {
    }

    void Release() override;
    void* const GetNative() const override;

private:
    ID3D12Resource*         m_Resource;
    bool                    m_IsBuffer;

    union {
        struct {
            DX12BuddyAllocator*     m_BufferHeap;
            HeapAllocInfo           m_BufferAlloc;
        } Buffer;
    };
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

            surface->Present();

            const u64 fence_value{ ++FenceValue };
            Frame& frame{ Frames[FrameIndex] };
            frame.FenceValue = fence_value;

            Queue->Signal(Fence, fence_value);
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
    Vector<ID3D12Resource*>             m_Resources;

    CommandQueue                        m_Queue;
    MemoryHeap                          m_Heap;
    DX12DescriptorHeap                  m_RtvHeap;
    DX12DescriptorHeap                  m_DsvHeap;

    struct {
        D3D12_CPU_DESCRIPTOR_HANDLE     CpuStart;
        D3D12_GPU_DESCRIPTOR_HANDLE     GpuStart;
        u32                             Start;
        u32                             Count;
    } m_SrvHeapData;
};
}
