#include <Iron.RHI/Renderer.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>

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
        IRHIFrameGraph** outHandle) override {
        UNREFERENCED_PARAMETER(builder);
        UNREFERENCED_PARAMETER(flags);
        UNREFERENCED_PARAMETER(outHandle);
        return Result::ENoInterface;
    }

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

    DX12BuddyAllocator              m_BufferHeaps;
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
    void* const GetNativeBuffer(u32 index) const override { return nullptr; }

private:
    IDXGISwapChain4*    m_Swapchain;
    ID3D12Resource*     m_Buffers[MaxBuffers];
    u32                 m_BufferCount;
    bool                m_SupportTearing;
};
}
