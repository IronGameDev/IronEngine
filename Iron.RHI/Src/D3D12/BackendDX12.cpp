#include <Iron.RHI/Src/D3D12/BackendDX12.h>
#include <Iron.RHI/Src/Shared/Shared.h>

#include <wrl.h>
#include <format>
#include <cassert>

using Microsoft::WRL::ComPtr;

namespace Iron::RHI::D3D12 {
namespace {
constexpr D3D12_HEAP_TYPE
ConvertHeapType(ResourceUsage::Usage usage) {
    switch (usage)
    {
    case ResourceUsage::Default:
        return D3D12_HEAP_TYPE_DEFAULT;
    case ResourceUsage::Immutable:
        return D3D12_HEAP_TYPE_DEFAULT;
    case ResourceUsage::Dynamic:
        return D3D12_HEAP_TYPE_DEFAULT;
    case ResourceUsage::Copy:
        return D3D12_HEAP_TYPE_UPLOAD;
    default:
        return D3D12_HEAP_TYPE_DEFAULT;
    }
}

constexpr D3D12_RESOURCE_DIMENSION
ConvertResourceDimension(ResourceDimension::Dim dim) {
    switch (dim)
    {
    case ResourceDimension::Buffer:
        return D3D12_RESOURCE_DIMENSION_BUFFER;
    case ResourceDimension::Texture1D:
        return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    case ResourceDimension::Texture2D:
        return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    case ResourceDimension::Texture3D:
        return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    default:
        return D3D12_RESOURCE_DIMENSION_UNKNOWN;
    }
}

constexpr D3D12_RESOURCE_FLAGS
ConvertResourceFlags(ResourceFlags::Flags flags) {
    D3D12_RESOURCE_FLAGS value{};
    if (!(flags & ResourceFlags::AllowShaderResource)) value |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    return value;
}

unsigned int CeilLog2(unsigned int x) {
    if (x == 0) {
        return 0;
    }
    unsigned int log2Val = 0;
    unsigned int temp = x - 1;
    while (temp > 0) {
        temp >>= 1;
        ++log2Val;
    }
    return log2Val;
}

inline bool
BBitTest(const std::vector<u64>& data, u32 idx) {
    return (data[idx >> 6] >> (idx & 63)) & 1ull;
}

inline void
BBitSet(std::vector<u64>& data, u32 idx) {
    data[idx >> 6] |= (1ull << (idx & 63));
}

inline void
BBitClear(std::vector<u64>& data, u32 idx) {
    data[idx >> 6] &= ~(1ull << (idx & 63));
}
}//anonymous namespace

bool
DX12SlabAllocator::Initialize(
    ID3D12Device14* const device,
    ResourceUsage::Usage heapUsage,
    u32 slabSize) {
    D3D12_HEAP_DESC desc{};
    desc.SizeInBytes = HeapSize;
    desc.Properties.Type = ConvertHeapType(heapUsage);
    desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    desc.Properties.CreationNodeMask = 0;
    desc.Properties.VisibleNodeMask = 0;
    desc.Alignment = Alignment64KB;
    desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    HRESULT hr{ S_OK };
    hr = device->CreateHeap(&desc, IID_PPV_ARGS(&m_Heap));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create gpu buffer buddy heap!");
        return false;
    }

    const u32 num_slots{ HeapSize / slabSize };
    m_FreeIndices.Resize(num_slots);

    for (u32 i{ 0 }; i < num_slots; ++i) {
        m_FreeIndices[i] = num_slots - i - 1;
    }

    return true;
}

void
DX12SlabAllocator::Release() {
    SafeRelease(m_Heap);
}

HeapAllocInfo
DX12SlabAllocator::Allocate(u32 size) {
    if (!m_FreeIndices.Size() && size <= m_SlabSize) {
        return {};
    }

    const u32 index{ *m_FreeIndices.end() };
    m_FreeIndices.PopBack();

    const u64 offset{ index * m_SlabSize };

    return { m_Heap, offset, size, 0, index };
}

void
DX12SlabAllocator::Free(const HeapAllocInfo& alloc) {
    m_FreeIndices.PushBack(alloc.Index);
}

bool
DX12BuddyAllocator::Initialize(
    ID3D12Device14* const device,
    ResourceUsage::Usage heapUsage) {
    D3D12_HEAP_DESC desc{};
    desc.SizeInBytes = HeapSize;
    desc.Properties.Type = ConvertHeapType(heapUsage);
    desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    desc.Properties.CreationNodeMask = 0;
    desc.Properties.VisibleNodeMask = 0;
    desc.Alignment = Alignment64KB;
    desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    HRESULT hr{ S_OK };
    hr = device->CreateHeap(&desc, IID_PPV_ARGS(&m_Heap));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create gpu buffer buddy heap!");
        return false;
    }

    for (u32 order = 0; order < OrderCount; ++order) {
        const u32 blocks{ MaxLeaves >> order };
        const u32 words{ (blocks + 63) / 64 };

        m_Free[order].resize(words);
        m_Split[order].resize(words);
    }

    m_Free[OrderCount - 1][0] = 1ull;

    return true;
}

void
DX12BuddyAllocator::Release() {
    SafeRelease(m_Heap);
}

HeapAllocInfo
DX12BuddyAllocator::Allocate(u64 sizeBytes) {
    if (sizeBytes == 0)
        return { nullptr, 0, 0 };

    // Align to 64KB
    u64 aligned = (sizeBytes + LeafSize - 1) & ~(LeafSize - 1);

    // Convert to leaf count
    u32 leaves = (u32)(aligned >> LeafLog2);

    // Compute order relative to leaf
    u32 order = 0;

    if (leaves > 1)
        order = 32 - std::countl_zero(leaves - 1);

    if (order >= OrderCount)
        return { nullptr, 0, 0 };

    u32 foundOrder = order;
    u32 index = 0;

    for (; foundOrder < OrderCount; ++foundOrder) {
        if (FindFreeBlock(foundOrder, index))
            break;
    }

    if (foundOrder == OrderCount)
        return { nullptr, 0, 0 };

    // Split downward
    while (foundOrder > order) {
        BBitClear(m_Free[foundOrder], index);
        BBitSet(m_Split[foundOrder], index);

        index <<= 1;
        BBitSet(m_Free[foundOrder - 1], index + 1);

        foundOrder--;
    }

    BBitClear(m_Free[order], index);

    u64 blockSize = LeafSize << order;
    u64 offset = (u64)index * blockSize;

    LOG_DEBUG("Allocated for buffer at: order=%u index=%u offset=%llu",
        order, index, offset);

    return { m_Heap, offset, blockSize, order, index };
}

void
DX12BuddyAllocator::Free(const HeapAllocInfo& alloc) {
#ifdef _DEBUG
    ValidateHandle(alloc);
#endif

    u32 order = alloc.Order;
    u32 index = alloc.Index;

    BBitSet(m_Free[order], index);

    while (order < OrderCount - 1) {
        u32 buddy = index ^ 1;
        u32 parent = index >> 1;

        // 1) Buddy must be free
        if (!BBitTest(m_Free[order], buddy))
            break;

        // 2) Parent must be marked split
        if (!BBitTest(m_Split[order + 1], parent))
            break;

        // Remove children from free list
        BBitClear(m_Free[order], index);
        BBitClear(m_Free[order], buddy);

        // Clear parent split flag
        BBitClear(m_Split[order + 1], parent);

        // Move up
        index = parent;
        order++;

        // Mark parent free
        BBitSet(m_Free[order], index);
    }
}

bool
DX12BuddyAllocator::FindFreeBlock(u32 order, u32& outIndex) {
    auto& words{ m_Free[order] };

    for (u32 w{ 0 }; w < words.size(); ++w) {
        const u64 v{ words[w] };
        if (v) {
            unsigned long bit{};
            _BitScanForward64(&bit, v);

            outIndex = (w << 6) + bit;
            return true;
        }
    }

    return false;
}

void
DX12BuddyAllocator::ValidateHandle(const HeapAllocInfo& alloc) {
    assert(alloc.Order < OrderCount);

    u32 maxBlocks = MaxLeaves >> alloc.Order;
    assert(alloc.Index < maxBlocks);

    // Block must NOT already be free
    assert(!BBitTest(m_Free[alloc.Order], alloc.Index));

    // Block must NOT be split
    assert(!BBitTest(m_Split[alloc.Order], alloc.Index));
}

CRHIDevice_DX12::CRHIDevice_DX12(
    IRHIFactory* const factory,
    IRHIAdapter* const adapter,
    const DeviceInitInfo& info)
    :
    m_Factory(factory),
    m_Debug(false),
    m_D3D12Dll(),
    m_D3D12CreateDevice(),
    m_D3D12GetDebugInterface(),
    m_FeatureLevel(),
    m_Device(),
    m_GraphicsQueue(),
    m_BufferHeaps() {

    if (!(adapter && m_Factory)) {
        LOG_ERROR("Invalid adapter/factory given to device!");
        return;
    }

    m_D3D12Dll = LoadLibraryExA("d3d12.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!m_D3D12Dll) {
        LOG_FATAL("Failed to load d3d12.dll!");
        return;
    }

    m_D3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(m_D3D12Dll, "D3D12CreateDevice");
    m_D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(m_D3D12Dll, "D3D12GetDebugInterface");
    if (!(m_D3D12CreateDevice && m_D3D12GetDebugInterface)) {
        LOG_FATAL("Failed to get D3D12CreateDevice() and/or D3D12GetDebugInterface() from d3d12.dll!");
        return;
    }

    if (info.Debug) {
        ComPtr<ID3D12Debug3> debug{};
        if (SUCCEEDED(m_D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
            LOG_DEBUG("Enabled debug for d3d12 device!");
        }
        m_Debug = true;
    }

    IUnknown* const dxgi_adapter{ (IUnknown* const)adapter->GetNative() };

    m_FeatureLevel = _GetMaximumFL(dxgi_adapter);
    if (!m_FeatureLevel) {
        LOG_FATAL("D3D12 Not supported!");
        return;
    }

    HRESULT hr{ S_OK };
    hr = m_D3D12CreateDevice(
        dxgi_adapter,
        m_FeatureLevel,
        IID_PPV_ARGS(&m_Device));
    if (FAILED(hr)) {
        LOG_FATAL("D3D12 Not supported!");
        return;
    }

    if (m_Debug) {
        ComPtr<ID3D12InfoQueue> info_queue{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&info_queue)))) {
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        }
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = info.DisableGPUTimeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;

    hr = m_Device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_GraphicsQueue));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create graphics command queue!");
        return;
    }

    m_BufferHeaps.Initialize(m_Device, ResourceUsage::Default);
}

void
CRHIDevice_DX12::Release() {
    m_BufferHeaps.Release();

    SafeRelease(m_GraphicsQueue);

    if (m_Debug) {
        {
            ComPtr<ID3D12InfoQueue> info_queue{};
            if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&info_queue)))) {
                info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
                info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
                info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
            }
        }

        ComPtr<ID3D12DebugDevice> debug_device{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&debug_device)))) {
            debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        }
    }

    SafeRelease(m_Device);

    delete this;
}

Result::Code
CRHIDevice_DX12::CreateResource(const ResourceInitInfo& info,
    IRHIResource** resource) {

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = ConvertResourceDimension(info.Dimension);
    desc.Alignment = 0;
    desc.Width = info.Width;
    desc.Height = info.Height;
    desc.DepthOrArraySize = (u16)info.DepthOrArray;
    desc.MipLevels = (u16)info.MipLevels;
    desc.Format = Shared::ConvertFormat(info.Format);
    desc.SampleDesc = { 1, 0 };
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = ConvertResourceFlags((ResourceFlags::Flags)info.Flags);

    D3D12_RESOURCE_ALLOCATION_INFO alloc_info{
        m_Device->GetResourceAllocationInfo(
            0,
            1,
            &desc
        )
    };

    switch (info.Dimension)
    {
    case ResourceDimension::Buffer: {
        HeapAllocInfo heap_info{ m_BufferHeaps.Allocate(alloc_info.SizeInBytes) };
        if (!(heap_info.Heap && heap_info.Size == alloc_info.SizeInBytes)) {
            return Result::ENomemory;
        }

        ID3D12Resource* res{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreatePlacedResource(
            heap_info.Heap,
            heap_info.Offset,
            &desc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&res));
        if (FAILED(hr)) {
            return Result::ECreateResource;
        }

        CRHIResource_DX12* temp{ new CRHIResource_DX12(res, &m_BufferHeaps, heap_info) };
        if (!temp) {
            return Result::ENomemory;
        }

        *resource = temp;
    } break;
    default:
        break;
    }

    return Result::Ok;
}

Result::Code
CRHIDevice_DX12::CreateSurface(const SurfaceInitInfo& info,
    IRHISurface** surface)
{
    CRHISurface_DX12* temp{ new CRHISurface_DX12(this, m_Factory, info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!temp->GetNative()) {
        delete temp;
        return Result::ECreateRHIObject;
    }

    *surface = temp;

    return Result::Ok;
}

void* const
CRHIDevice_DX12::GetNative() const {
    return m_Device;
}

D3D_FEATURE_LEVEL
CRHIDevice_DX12::_GetMaximumFL(IUnknown* const adapter) const
{
    ComPtr<ID3D12Device> device{};
    if (FAILED(m_D3D12CreateDevice(adapter,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device))))
        return (D3D_FEATURE_LEVEL)0;

    const D3D_FEATURE_LEVEL levels[]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_2,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS data{};
    data.NumFeatureLevels = _countof(levels);
    data.pFeatureLevelsRequested = &levels[0];

    device->CheckFeatureSupport(
        D3D12_FEATURE_FEATURE_LEVELS,
        &data,
        sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));

    return data.MaxSupportedFeatureLevel;
}

void
CRHIResource_DX12::Release() {
    SafeRelease(m_Resource);

    if (m_IsBuffer) {
        if (!Buffer.m_BufferHeap) {
            LOG_ERROR("Released resource should ALWAYS have a heap!");
            return;
        }

        Buffer.m_BufferHeap->Free(Buffer.m_BufferAlloc);
    }

    delete this;
}

void* const
CRHIResource_DX12::GetNative() const {
    return m_Resource;
}

CRHISurface_DX12::CRHISurface_DX12(
    CRHIDevice_DX12* const device,
    IRHIFactory* const factory,
    const SurfaceInitInfo& info)
    : m_Swapchain(),
    m_Buffers(),
    m_BufferCount(info.TripleBuffering ? 3 : 2),
    m_SupportTearing() {
    if (!(device && factory &&
        info.Native && device->GetGraphicsQueue()
        && factory->GetNative())) {
        return;
    }

    const HWND hwnd{ (const HWND)info.Native };
    IDXGIFactory7* const dxgi{ (IDXGIFactory7* const)factory->GetNative() };
    m_SupportTearing = factory->SupportsTearing() && info.AllowTearing;

    DXGI_SWAP_CHAIN_DESC1 swap_desc{};
    swap_desc.Width = info.Width;
    swap_desc.Height = info.Height;
    swap_desc.Format = Shared::ConvertFormat(info.Format);
    swap_desc.Stereo = FALSE;
    swap_desc.SampleDesc = { 1, 0 };
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = m_BufferCount;
    swap_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_desc.Flags = m_SupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> temp{};

    HRESULT hr{ S_OK };
    hr = dxgi->CreateSwapChainForHwnd(
        device->GetGraphicsQueue(),
        hwnd,
        &swap_desc,
        nullptr,
        nullptr,
        &temp);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create swapchain!");
        return;
    }

    temp->QueryInterface(IID_PPV_ARGS(&m_Swapchain));

    for (u32 i{ 0 }; i < m_BufferCount; ++i) {
        m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Buffers[i]));
        if (!m_Buffers[i]) {
            LOG_ERROR("Failed to get swapchain buffers!");
            return;
        }
    }
}

void
CRHISurface_DX12::Release() {
    for (u32 i{ 0 }; i < m_BufferCount; ++i) {
        SafeRelease(m_Buffers[i]);
    }

    SafeRelease(m_Swapchain);

    delete this;
}

u32
CRHISurface_DX12::GetBufferCount() const {
    return m_BufferCount;
}

void* const
CRHISurface_DX12::GetNative() const {
    return m_Swapchain;
}
}
