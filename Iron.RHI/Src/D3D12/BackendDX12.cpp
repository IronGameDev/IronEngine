#include <Iron.RHI/Src/D3D12/BackendDX12.h>
#include <Iron.RHI/Src/Shared/Shared.h>

#include <wrl.h>
#include <format>
#include <cassert>
#include <unordered_map>
#include <algorithm>
#include <queue>

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

constexpr D3D12_RESOURCE_DIMENSION
ConvertResourceDimension(FGResourceType::Type type) {
    return type == FGResourceType::Texture
        ? D3D12_RESOURCE_DIMENSION_TEXTURE2D
        : (type == FGResourceType::Buffer)
        ? D3D12_RESOURCE_DIMENSION_BUFFER
        : D3D12_RESOURCE_DIMENSION_UNKNOWN;
}

constexpr D3D12_RESOURCE_FLAGS
ConvertResourceFlags(ResourceFlags::Flags flags) {
    D3D12_RESOURCE_FLAGS value{};
    if (!(flags & ResourceFlags::AllowShaderResource)) value |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    return value;
}


constexpr static D3D12_RESOURCE_STATES
ConvertResourceStates(ResourceState::State state)
{
    D3D12_RESOURCE_STATES val{};
    if (state & ResourceState::VertexBuffer) val |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (state & ResourceState::IndexBuffer) val |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    if (state & ResourceState::ConstantBuffer) val |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (state & ResourceState::RenderTarget) val |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    if (state & ResourceState::UnorderedAccess) val |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    if (state & ResourceState::DepthRead) val |= D3D12_RESOURCE_STATE_DEPTH_READ;
    if (state & ResourceState::DepthWrite) val |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    if (state & ResourceState::PixelResource) val |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if (state & ResourceState::NonPixelResource) val |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (state & ResourceState::IndirectArgs) val |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    if (state & ResourceState::CopyDest) val |= D3D12_RESOURCE_STATE_COPY_DEST;
    if (state & ResourceState::CopySrc) val |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    if (state & ResourceState::Present) val |= D3D12_RESOURCE_STATE_PRESENT;

    return val;
}

constexpr static D3D12_SHADER_VISIBILITY
ConvertVisibility(ShaderType::Type vis) {
    switch (vis)     {
    case ShaderType::Vertex:
        return D3D12_SHADER_VISIBILITY_VERTEX;
    case ShaderType::Pixel:
        return D3D12_SHADER_VISIBILITY_PIXEL;
    case ShaderType::Compute:
        return D3D12_SHADER_VISIBILITY_ALL;
    default:
        return D3D12_SHADER_VISIBILITY_ALL;
    }
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

std::wstring
ToWideString(std::string str) {
    return { str.begin(), str.end() };
}

namespace Cmd {
struct CommandListData {
    ID3D12GraphicsCommandList10*    List{};

    bool                            GraphicsLayout{};

    u32                             CounterDraw;
    u32                             CounterDrawInstanced;
    u32                             CounterDrawIndexed;
    u32                             CounterDrawIndexedInstanced;
};

typedef void(*CommandFunc)(CommandListData&, const void*);

void
SetGraphicsLayout(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetGraphicsLayoutInfo& info{ *(const RHICommandBuilder::CmdSetGraphicsLayoutInfo*)data };
    cmdData.List->SetGraphicsRootSignature((ID3D12RootSignature*)info.Layout->GetNative());
    cmdData.GraphicsLayout = true;
}

void
SetComputeLayout(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetGraphicsLayoutInfo& info{ *(const RHICommandBuilder::CmdSetGraphicsLayoutInfo*)data };
    cmdData.List->SetComputeRootSignature((ID3D12RootSignature*)info.Layout->GetNative());
    cmdData.GraphicsLayout = false;
}

void
Draw(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawInfo& info{ *(const RHICommandBuilder::CmdDrawInfo*)data };
    cmdData.List->DrawInstanced(info.VertexCount, 1, info.BaseVertex, 0);
    ++cmdData.CounterDraw;
}

void
DrawInstanced(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawInstancedInfo& info{ *(const RHICommandBuilder::CmdDrawInstancedInfo*)data };
    cmdData.List->DrawInstanced(info.VertexCount, info.InstanceCount, info.BaseVertex, info.BaseInstance);
    ++cmdData.CounterDrawInstanced;
}

void
DrawIndexed(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawIndexedInfo& info{ *(const RHICommandBuilder::CmdDrawIndexedInfo*)data };
    cmdData.List->DrawIndexedInstanced(info.IndexCount, 1, info.BaseIndex, info.BaseVertex, 0);
    ++cmdData.CounterDrawIndexed;
}

void
DrawIndexedInstanced(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawInstancedIndexedInfo& info{ *(const RHICommandBuilder::CmdDrawInstancedIndexedInfo*)data };
    cmdData.List->DrawIndexedInstanced(info.IndexCount, info.InstanceCount, info.BaseIndex, info.BaseVertex, info.BaseInstance);
    ++cmdData.CounterDrawIndexedInstanced;
}

constexpr static CommandFunc DispatchTable[RHICommandBuilder::CommandId::Count]{
    nullptr,
    SetGraphicsLayout,
    SetComputeLayout,
    Draw,
    DrawInstanced,
    DrawIndexed,
    DrawIndexedInstanced,
};

void
ParseCommandStream(RHICommandBuilder& builder, CommandListData& cmdData) {
    if (!(builder.GetStream() && cmdData.List)) {
        return;
    }

    const u32 total_size{ builder.GetOffset() };
    const u8* stream{ builder.GetStream() };
    const u8* const end{ stream + total_size };

    while (stream < end) {
        const RHICommandBuilder::CmdHeader header{ *(const RHICommandBuilder::CmdHeader*)stream };
        if (header.Id >= RHICommandBuilder::CommandId::Count) {
            LOG_ERROR("Invalid command id used!");
        }

        stream += sizeof(RHICommandBuilder::CmdHeader);
        DispatchTable[header.Id](cmdData, stream);
        stream += header.PayloadSize;
    }
}
}//cmd namespace
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

Result::Code
DX12DescriptorHeap::Initialize(ID3D12Device14* const device,
    u32 size,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    bool shader_visible) {
    if (!device) {
        LOG_ERROR("D3D12: Nullptr passed to CRHIDescriptorHeap_DX12::Create()!");
        return Result::ENullptr;
    }

    m_Type = type;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = size;
    desc.Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    HRESULT hr{ S_OK };
    hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap));
    if (FAILED(hr)) {
        LOG_ERROR("D3D12: Failed to create descriptor heap!");
        return Result::ECreateRHIObject;
    }

    m_Capacity = size;
    m_ShaderVisible = shader_visible;
    m_CpuStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
    if (m_ShaderVisible) {
        m_GpuStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
    }
    m_IncrementSize = device->GetDescriptorHandleIncrementSize(type);

    m_FreeList.Resize(m_Capacity);
    for (u32 i = 0; i < m_Capacity; ++i)
        m_FreeList[i] = i;

    LOG_INFO("D3D12: Created descriptor heap Size=%u, ShaderVisible=%u", m_Capacity, (u32)m_ShaderVisible);

    return Result::Ok;
}

void
DX12DescriptorHeap::Release() {
    SafeRelease(m_Heap);
    LOG_INFO("D3D12: Destroyed descriptor heap Size=%u, ShaderVisible=%u", m_Capacity, (u32)m_ShaderVisible);
}

u32
DX12DescriptorHeap::Allocate() {
    if (m_FreeList.Empty())
        return (u32)~0;

    u32 idx{ m_FreeList.Back() };
    m_FreeList.PopBack();
    return idx;
}

u32
DX12DescriptorHeap::Allocate(u32 count) {
    if (count == 0 || m_FreeList.Size() < count)
        return (u32)~0;

    for (u32 i{ 0 }; i + count <= m_FreeList.Size(); ++i) {
        u32 base = m_FreeList[i];
        bool contiguous = true;

        for (u32 j = 1; j < count; ++j) {
            if (m_FreeList[i + j] != base + j) {
                contiguous = false;
                break;
            }
        }

        if (contiguous) {
            m_FreeList.Erase(
                m_FreeList.begin() + i,
                m_FreeList.begin() + i + count
            );

            return base;
        }
    }

    return (u32)~0;
}

void
DX12DescriptorHeap::Free(u32 index, u64 fence_value) {
    m_PendingFrees.push_back({ fence_value, index });
}

void
DX12DescriptorHeap::Free(u32 base, u32 count, u64 fence_value) {
    for (u32 i = 0; i < count; ++i)
        m_PendingFrees.push_back({ fence_value, base + i });
}

void
DX12DescriptorHeap::Collect(u64 fence) {
    while (!m_PendingFrees.empty()) {
        auto& pf = m_PendingFrees.front();
        if (pf.FenceValue == fence) {
            m_FreeList.PushBack(pf.Index);
            m_PendingFrees.pop_front();
        }
        else
            break;
    }

    std::sort(m_FreeList.begin(), m_FreeList.end());
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
    m_BufferHeaps(),
    m_SrvHeap(),
    m_SerializeRootSig(),
    m_Features() {

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
    m_SerializeRootSig = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(m_D3D12Dll, "D3D12SerializeVersionedRootSignature");
    if (!(m_D3D12CreateDevice && m_D3D12GetDebugInterface
        && m_SerializeRootSig)) {
        LOG_FATAL("Failed to get D3D12CreateDevice() and/or D3D12GetDebugInterface() and/or D3D12SerializeVersionedRootSignature() from d3d12.dll!");
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
    m_SrvHeap.Initialize(m_Device, info.MaxShaderResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12_options{ QueryFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS>() };
    D3D12_FEATURE_DATA_SHADER_MODEL shader_model{ QueryFeatureSupport<D3D12_FEATURE_DATA_SHADER_MODEL>() };

    m_Features.Bindless = (d3d12_options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3
        && shader_model.HighestShaderModel >= D3D_SHADER_MODEL_6_0);
    m_Features.PushConstants = true;
    m_Features.ShaderModel.Major = (shader_model.HighestShaderModel & 0xf0) >> 4;
    m_Features.ShaderModel.Minor = shader_model.HighestShaderModel & 0xf;
    m_Features.FeatureLevel.Major = ((m_FeatureLevel >> 8) & 0xf0) >> 4;
    m_Features.FeatureLevel.Minor = (m_FeatureLevel >> 8) & 0xf;
}

void
CRHIDevice_DX12::Release() {
    m_BufferHeaps.Release();
    m_SrvHeap.Release();

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
CRHIDevice_DX12::CreatePipelineLayout(const PipelineLayoutInitInfo& info,
    IRHIPipelineLayout** outHandle) {
    CRHIPipelineLayout_DX12* temp{ new CRHIPipelineLayout_DX12(this, info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!temp->GetNative()) {
        delete temp;
        return Result::ECreateRHIObject;
    }

    *outHandle = temp;

    return Result::Ok;
}

Result::Code
CRHIDevice_DX12::CreateSurface(const SurfaceInitInfo& info,
    IRHISurface** surface) {
    CRHISurface_DX12* temp{ new CRHISurface_DX12(this, m_Factory, info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!(temp->GetNative() && temp->GetNativeBuffer(0))) {
        delete temp;
        return Result::ECreateRHIObject;
    }

    *surface = temp;

    return Result::Ok;
}

Result::Code
CRHIDevice_DX12::CreateFrameGraph(const RHIGraphBuilder& builder,
    u32 flags,
    IRHIFrameGraph** outHandle) {
    CRHIFrameGraph_DX12* temp{ new CRHIFrameGraph_DX12() };
    if (!temp) {
        return Result::ENomemory;
    }

    Result::Code res{ Result::Ok };
    res = temp->Initialize(this, flags, builder);

    if (Result::Fail(res)) {
        delete temp;
        return res;
    }

    *outHandle = temp;

    return Result::Ok;
}

void
CRHIDevice_DX12::GetFeatures(
    DeviceFeatures* features) {
    if (!features)
        return;

    *features = m_Features;
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

CRHIPipelineLayout_DX12::CRHIPipelineLayout_DX12(
    CRHIDevice_DX12* const device,
    const PipelineLayoutInitInfo& info)
    : m_RootSig() {
    if (!(device && device->GetNative())) {
        return;
    }

    std::vector<D3D12_ROOT_PARAMETER> rootParams;
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;

    for (u32 i{ 0 }; i < info.NumParams; ++i) {
        const auto& param = info.Params[i];

        D3D12_ROOT_PARAMETER root{};
        root.ShaderVisibility = ConvertVisibility(param.Visibility);

        bool isTable = param.Count > 1 || param.Bindless;

        if (!isTable) {
            switch (param.Type) {
            case PipelineLayoutParamType::ConstantBuffer:
                root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                root.Descriptor.ShaderRegister = param.Slot;
                root.Descriptor.RegisterSpace = param.Space;
                break;

            case PipelineLayoutParamType::ShaderResource:
                root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
                root.Descriptor.ShaderRegister = param.Slot;
                root.Descriptor.RegisterSpace = param.Space;
                break;

            case PipelineLayoutParamType::UnorderedAccess:
                root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
                root.Descriptor.ShaderRegister = param.Slot;
                root.Descriptor.RegisterSpace = param.Space;
                break;
            }

            rootParams.push_back(root);
        }
        else {
            D3D12_DESCRIPTOR_RANGE range{};
            range.BaseShaderRegister = param.Slot;
            range.RegisterSpace = param.Space;
            range.NumDescriptors = param.Bindless ? UINT_MAX : param.Count;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            switch (param.Type) {
            case PipelineLayoutParamType::ShaderResource:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;

            case PipelineLayoutParamType::ConstantBuffer:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                break;

            case PipelineLayoutParamType::UnorderedAccess:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;

            case PipelineLayoutParamType::Sampler:
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                break;
            }

            ranges.push_back(range);

            root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            root.DescriptorTable.NumDescriptorRanges = 1;
            root.DescriptorTable.pDescriptorRanges = &ranges.back();

            rootParams.push_back(root);
        }
    }

    if (info.PushConstantSize)
    {
        D3D12_ROOT_PARAMETER root{};
        root.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        root.Constants.Num32BitValues = info.PushConstantSize / 4;
        root.Constants.ShaderRegister = 0;
        root.Constants.RegisterSpace = 0;
        root.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParams.push_back(root);
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc{};
    desc.Version = D3D_ROOT_SIGNATURE_VERSION_1;
    desc.Desc_1_0.NumParameters = (UINT)rootParams.size();
    desc.Desc_1_0.pParameters = rootParams.data();
    desc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    HRESULT hr{ S_OK };
    hr = device->D3D12SerializeVersionedRootSignature(
        &desc,
        &blob,
        &error);
    if (FAILED(hr)) {
        if (error) {
            LOG_ERROR("RootSig Error: %s", (char*)error->GetBufferPointer());
        }

        return;
    }

    ID3D12Device14* const d3d12{ (ID3D12Device14* const)device->GetNative() };

    hr = d3d12->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&m_RootSig)
    );
}

void
CRHIPipelineLayout_DX12::Release() {
    SafeRelease(m_RootSig);
}

void* const
CRHIPipelineLayout_DX12::GetNative() const {
    return m_RootSig;
}

CRHISurface_DX12::CRHISurface_DX12(
    CRHIDevice_DX12* const device,
    IRHIFactory* const factory,
    const SurfaceInitInfo& info)
    : m_Swapchain(),
    m_Buffers(),
    m_BufferCount(info.TripleBuffering ? 3 : 2),
    m_SupportTearing(),
    m_RtvHeap(),
    m_BaseDescriptor(),
    m_DescriptorSize(),
    m_CurrentBB() {
    if (!(device && factory &&
        info.Native && device->GetGraphicsQueue()
        && factory->GetNative())) {
        return;
    }

    const HWND hwnd{ (const HWND)info.Native };
    IDXGIFactory7* const dxgi{ (IDXGIFactory7* const)factory->GetNative() };
    ID3D12Device14* const d3d12{ (ID3D12Device14* const)device->GetNative() };
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

    D3D12_DESCRIPTOR_HEAP_DESC heap{};
    heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap.NumDescriptors = m_BufferCount;
    heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap.NodeMask = 0;

    hr = d3d12->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&m_RtvHeap));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create surface rtv heap!");
        return;
    }

    m_BaseDescriptor = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_DescriptorSize = d3d12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    Finalize(d3d12);
}

void
CRHISurface_DX12::Release() {
    SafeRelease(m_RtvHeap);

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

void* const
CRHISurface_DX12::GetNativeBuffer(u32 index) const {
    if (index >= m_BufferCount)
        return nullptr;

    return m_Buffers[index];
}

void
CRHISurface_DX12::Present(bool vsync) {
    m_Swapchain->Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
    m_CurrentBB = m_Swapchain->GetCurrentBackBufferIndex();
}

u32
CRHISurface_DX12::GetCurrentBBIndex() const {
    return m_CurrentBB;
}

D3D12_CPU_DESCRIPTOR_HANDLE
CRHISurface_DX12::GetBufferDescriptor(u32 index) {
    if (index >= m_BufferCount)
        return {};

    return { m_BaseDescriptor.ptr + (index * m_DescriptorSize) };
}

void
CRHISurface_DX12::Finalize(ID3D12Device14* const d3d12) {
    for (u32 i{ 0 }; i < m_BufferCount; ++i) {
        m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Buffers[i]));
        if (!m_Buffers[i]) {
            LOG_ERROR("Failed to get swapchain buffers!");
            return;
        }

        d3d12->CreateRenderTargetView(
            m_Buffers[i],
            nullptr,
            GetBufferDescriptor(i));
    }

    m_CurrentBB = m_Swapchain->GetCurrentBackBufferIndex();
}

Result::Code
CRHIFrameGraph_DX12::Initialize(
    CRHIDevice_DX12* const device,
    u32 flags,
    const RHIGraphBuilder& builder) {
    if (!device) {
        return Result::ENullptr;
    }

    const Vector<Vector<u32>> dependencies{ GetDependencies(builder) };
    if ((u32)(flags & FGCompileFlags::LogInfo)) {
        PrintDependencies(dependencies);
    }

    ID3D12Device14* const d3d12{ (ID3D12Device14* const)device->GetNative() };
    const Vector<u32> sorted{ TopologicalSort(dependencies) };
    const Vector<RHIGraphBuilder::FGPassDesc>& passes{ builder.GetPasses() };
    const Vector<FGResourceInitInfo>& resources{ builder.GetResources() };
    const bool debug_names{ (const bool)(flags & FGCompileFlags::DebugNames) };

    {
        Result::Code res{ m_Queue.Initialize(
            d3d12,
            device->GetGraphicsQueue(),
            D3D12_COMMAND_LIST_TYPE_DIRECT) };
        if (Result::Fail(res)) {
            return res;
        }
    }

    m_Passes.Resize(sorted.Size());
    for (u32 i{ 0 }; i < m_Passes.Size(); ++i) {
        m_Passes[i].Func = passes[sorted[i]].Func;
    }

    using namespace Shared;

    m_DescResources.Resize(resources.Size());
    Vector<D3D12_RESOURCE_DESC> resource_descs{};

    u32 total_count{};

    for (u32 i{ 0 }; i < resources.Size(); ++i) {
        const FGResourceInitInfo& info{ resources[i] };

        if (info.Type == FGResourceType::Swapchain) {
            Resource res{};
            res.Start = 0;
            res.Count = (u16)info.TemporalCount;
            res.Type = CompiledType::Swapchain;

            m_DescResources[i] = res;
            continue;
        }

        const u8 info_flags{ info.GetFlags() };
        const CompiledType::Type compiled_type{ info.Type == FGResourceType::Texture
            ? CompiledType::Texture : CompiledType::Buffer };

        D3D12_RESOURCE_FLAGS bind_flags{};
        if (!(info_flags & RHIGraphBuilder::ResourceFlag_SRV)) {
            bind_flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
        if (info_flags & RHIGraphBuilder::ResourceFlag_RTV) {
            bind_flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (info_flags & RHIGraphBuilder::ResourceFlag_DSV) {
            bind_flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (info_flags & RHIGraphBuilder::ResourceFlag_UAV) {
            bind_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = ConvertResourceDimension(info.Type);
        desc.Alignment = 0;
        desc.Width = info.Width;
        desc.Height = info.Height;
        desc.DepthOrArraySize = (u16)info.DepthOrArray;
        desc.MipLevels = (u16)info.MipLevels;
        desc.Format = ConvertFormat(info.StorageFormat);
        desc.SampleDesc = { 1, 0 };
        desc.Layout = compiled_type == CompiledType::Texture
            ? D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE
            : D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = bind_flags;

        resource_descs.PushBack(desc);

        Resource res{};
        res.Start = (u16)total_count;
        res.Count = (u16)info.TemporalCount;
        res.Type = compiled_type;

        m_DescResources[i] = res;
        total_count += info.TemporalCount;
    }

    m_Resources.Resize(total_count);
    D3D12_RESOURCE_ALLOCATION_INFO alloc_info{
        d3d12->GetResourceAllocationInfo(0,
            (u32)resource_descs.Size(),
            resource_descs.Data())
    };

    if (!alloc_info.SizeInBytes) {
        LOG_ERROR("No memory requested by frame graph!");
        return Result::EFrameGraph;
    }

    m_Heap.Initialize(d3d12, alloc_info.SizeInBytes, alloc_info.Alignment);

    Vector<D3D12_RESOURCE_STATES> last_states{};
    last_states.Resize(m_DescResources.Size());

    {
        u64 heap_offset{};

        for (u32 i{ 0 }; i < resources.Size(); ++i) {
            const Resource& res{ m_DescResources[i] };
            if (res.Type == CompiledType::Swapchain) {
                last_states[i] = D3D12_RESOURCE_STATE_PRESENT;
                continue;
            }

            const D3D12_RESOURCE_DESC& desc{ resource_descs[i] };
            const D3D12_RESOURCE_STATES last_state{ ConvertResourceStates(resources[i].GetLastState()) };
            last_states[i] = last_state;

            for (u32 temporal{ 0 }; temporal < res.Count; ++temporal) {
                HRESULT hr{ S_OK };
                hr = d3d12->CreatePlacedResource(
                    m_Heap.Heap,
                    heap_offset,
                    &desc,
                    last_state,
                    nullptr,
                    IID_PPV_ARGS(&m_Resources[res.Start + temporal]));
                if (FAILED(hr)) {
                    LOG_ERROR("Failed to create graph resource!");
                    return Result::ECreateResource;
                }

                if (debug_names) {
                    m_Resources[res.Start + temporal]->SetName(
                        ToWideString(resources[i].Name).c_str()
                    );
                }

                MemFree((void*)resources[i].Name);

                D3D12_RESOURCE_ALLOCATION_INFO resource_info{ d3d12->GetResourceAllocationInfo(0, 1, &desc) };

                heap_offset += Math::AlignUp(resource_info.SizeInBytes, resource_info.Alignment);
            }
        }
    }

    const u32 num_srvs{ builder.GetSRViewCounter() };
    const u32 num_rtvs{ builder.GetRTViewCounter() };
    const u32 num_dsvs{ builder.GetDSViewCounter() };

    DX12DescriptorHeap& srv_heap{ device->GetSrvHeap() };
    m_SrvHeapData.Start = srv_heap.Allocate(num_srvs);
    m_SrvHeapData.Count = num_srvs;

    if (m_SrvHeapData.Start == (u32)~0) {
        LOG_ERROR("Failed to allocate graph descriptors!");
        return Result::ECreateRHIObject;
    }

    m_SrvHeapData.CpuStart = srv_heap.GetCpuHandle(m_SrvHeapData.Start);
    m_SrvHeapData.GpuStart = srv_heap.GetGpuHandle(m_SrvHeapData.Start);

    Result::Code res{ Result::Ok };
    res = m_RtvHeap.Initialize(d3d12, num_rtvs, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
    if (Result::Fail(res)) {
        LOG_ERROR("Failed to allocate graph descriptors!");
        return Result::ECreateRHIObject;
    }

    res = m_DsvHeap.Initialize(d3d12, num_dsvs, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);
    if (Result::Fail(res)) {
        LOG_ERROR("Failed to allocate graph descriptors!");
        return Result::ECreateRHIObject;
    }

    std::unordered_map<ViewCacheEntry, u32, ViewCacheHasher> view_cache;

    u16 srv_heap_index{ 0 };
    u16 rtv_heap_index{ 0 };
    u16 dsv_heap_index{ 0 };

    for (u32 i{ 0 }; i < (u32)m_Passes.Size(); ++i) {
        const RHIGraphBuilder::FGPassDesc& pass_desc{ passes[i] };
        Pass& compiled{ m_Passes[i] };

        for (u32 j{ 0 }; j < (u32)pass_desc.Reads.Size(); ++j) {
            const auto& read{ pass_desc.Reads[j] };
            const u32 slot{ read.Slot };
            const Resource& resource{ m_DescResources[read.Resource] };

            //TODO: Handle barriers
            if (resource.Type == CompiledType::Swapchain) {
                continue;
            }

            ViewCacheEntry entry{};
            entry.Resource = read.Resource;
            entry.Format = read.ViewFormat;
            entry.BaseMip = read.Range.BaseMip;
            entry.MipCount = read.Range.MipCount;
            entry.BaseLayer = read.Range.BaseLayer;
            entry.LayerCount = read.Range.LayerCount;
            entry.Plane = read.Range.Plane;

            const D3D12_RESOURCE_STATES new_state{ ConvertResourceStates(read.State) };

            BarrierDesc barrier{};
            barrier.Resource = read.Resource;
            barrier.Before = last_states[read.Resource];
            barrier.After = new_state;
            last_states[read.Resource] = new_state;

            auto it = view_cache.find(entry);
            if (it == view_cache.end()) {
                if (read.State & ResourceState::PixelResource
                    || read.State & ResourceState::NonPixelResource) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::ShaderResource;
                    view.BaseIndex = srv_heap_index;

                    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
                    desc.Format = ConvertFormat(entry.Format);
                    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    desc.Texture2D.MostDetailedMip = entry.BaseMip;
                    desc.Texture2D.MipLevels = entry.MipCount;
                    desc.Texture2D.PlaneSlice = entry.Plane;
                    desc.Texture2D.ResourceMinLODClamp = 0.f;

                    for (u32 temporal{ 0 }; temporal < resource.Count; ++temporal) {
                        d3d12->CreateShaderResourceView(
                            m_Resources[resource.Start + temporal],
                            &desc,
                            srv_heap.GetCpuHandle(m_SrvHeapData.Start + srv_heap_index + temporal)
                        );
                    }

                    srv_heap_index += resource.Count;

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    BoundView binding{};
                    binding.View = (u16)view_id;
                    binding.Slot = (u16)slot;
                    compiled.Srvs.PushBack(binding);
                }
                else if (read.State & ResourceState::DepthRead) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::DepthStencil;
                    view.BaseIndex = dsv_heap_index;

                    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
                    desc.Format = ConvertFormat(entry.Format);
                    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH
                        | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
                    desc.Texture2D.MipSlice = entry.BaseMip;

                    for (u32 temporal{ 0 }; temporal < resource.Count; ++temporal) {
                        d3d12->CreateDepthStencilView(
                            m_Resources[resource.Start + temporal],
                            &desc,
                            m_DsvHeap.GetCpuHandle(dsv_heap_index + temporal)
                        );
                    }

                    dsv_heap_index += resource.Count;

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)view_id;
                    compiled.Dsv.Slot = 0;
                }
            }
            else {
                if (read.State & ResourceState::PixelResource
                    || read.State & ResourceState::NonPixelResource) {
                    BoundView binding{};
                    binding.View = (u16)it->second;
                    binding.Slot = (u16)slot;
                    compiled.Srvs.PushBack(binding);
                }
                else if (read.State & ResourceState::DepthRead) {
                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)it->second;
                    compiled.Dsv.Slot = (u16)slot;
                }
            }
        }

        for (u32 j{ 0 }; j < (u32)pass_desc.Writes.Size(); ++j) {
            const auto& write{ pass_desc.Writes[j] };
            const u32 slot{ write.Slot };
            const Resource& resource{ m_DescResources[write.Resource] };

            if (write.ClearOp == FGClearOp::RenderTarget) {
                compiled.RtvClearValues[slot].X = write.ClearValue.Color[0];
                compiled.RtvClearValues[slot].Y = write.ClearValue.Color[1];
                compiled.RtvClearValues[slot].Z = write.ClearValue.Color[2];
                compiled.RtvClearValues[slot].W = write.ClearValue.Color[3];
                compiled.EnableClearTarget((u16)slot);
            }
            else if (write.ClearOp == FGClearOp::DepthStencil) {
                compiled.DepthClearValue.Depth = write.ClearValue.Depth.Depth;
                compiled.DepthClearValue.Stencil = write.ClearValue.Depth.Stencil;
                compiled.EnableClearDepth();
            }

            if (resource.Type == CompiledType::Swapchain) {
                const D3D12_RESOURCE_STATES new_state{ ConvertResourceStates(write.State) };

                BarrierDesc barrier{};
                barrier.Resource = (u32)~0;
                barrier.Before = last_states[write.Resource];
                barrier.After = new_state;
                last_states[write.Resource] = new_state;

                compiled.Barriers.PushBack(barrier);

                compiled.NumRtvs++;
                compiled.Rtvs[slot].Slot = (u16)slot;
                compiled.Rtvs[slot].View = (u16)~0;
                continue;
            }

            ViewCacheEntry entry{};
            entry.Resource = write.Resource;
            entry.Format = write.ViewFormat;
            entry.BaseMip = write.Range.BaseMip;
            entry.MipCount = write.Range.MipCount;
            entry.BaseLayer = write.Range.BaseLayer;
            entry.LayerCount = write.Range.LayerCount;
            entry.Plane = write.Range.Plane;

            const D3D12_RESOURCE_STATES new_state{ ConvertResourceStates(write.State) };

            BarrierDesc barrier{};
            barrier.Resource = write.Resource;
            barrier.Before = last_states[write.Resource];
            barrier.After = new_state;
            last_states[write.Resource] = new_state;

            compiled.Barriers.PushBack(barrier);

            auto it = view_cache.find(entry);
            if (it == view_cache.end()) {
                if (write.State & ResourceState::RenderTarget) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::RenderTarget;
                    view.BaseIndex = rtv_heap_index;

                    D3D12_RENDER_TARGET_VIEW_DESC desc{};
                    desc.Format = ConvertFormat(entry.Format);
                    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    desc.Texture2D.MipSlice = entry.BaseMip;
                    desc.Texture2D.PlaneSlice = entry.Plane;

                    for (u32 temporal{ 0 }; temporal < resource.Count; ++temporal) {
                        d3d12->CreateRenderTargetView(
                            m_Resources[resource.Start + temporal],
                            &desc,
                            m_RtvHeap.GetCpuHandle(rtv_heap_index + temporal)
                        );
                    }

                    rtv_heap_index += resource.Count;

                    const u32 view_id{ m_DescViews.Size() };

                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.NumRtvs++;
                    compiled.Rtvs[slot].Slot = (u16)slot;
                    compiled.Rtvs[slot].View = (u16)view_id;
                }
                else if (write.State & ResourceState::DepthWrite) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::DepthStencil;
                    view.BaseIndex = dsv_heap_index;

                    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
                    desc.Format = ConvertFormat(entry.Format);
                    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    desc.Flags = D3D12_DSV_FLAG_NONE;
                    desc.Texture2D.MipSlice = entry.BaseMip;

                    for (u32 temporal{ 0 }; temporal < resource.Count; ++temporal) {
                        d3d12->CreateDepthStencilView(
                            m_Resources[resource.Start + temporal],
                            &desc,
                            m_DsvHeap.GetCpuHandle(dsv_heap_index + temporal)
                        );
                    }

                    dsv_heap_index += resource.Count;

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)view_id;
                    compiled.Dsv.Slot = 0;
                }
            }
            else {
                if (write.State & ResourceState::RenderTarget) {
                    compiled.NumRtvs++;
                    compiled.Rtvs[slot].Slot = (u16)slot;
                    compiled.Rtvs[slot].View = (u16)it->second;
                }
                else if (write.State & ResourceState::DepthRead) {
                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)it->second;
                    compiled.Dsv.Slot = (u16)write.Slot;
                }
            }
        }
    }

    return Result::Ok;
}

void
CRHIFrameGraph_DX12::Release() {
    m_Queue.Release();
    m_RtvHeap.Release();
    m_DsvHeap.Release();

    for (auto& res : m_Resources) {
        SafeRelease(res);
    }

    m_Heap.Release();
}

void
CRHIFrameGraph_DX12::Execute(
    IRHISurface* const surface,
    u64 frameNumber) {
    CRHISurface_DX12* const dx_surface{ (CRHISurface_DX12* const)surface };

    m_Queue.Begin();

    Cmd::CommandListData cmd_data{};
    cmd_data.List = m_Queue.List;
    //Dont realloc every frame!
    RHICommandBuilder builder{ 1024 * 1024 };
    builder.Reset();

    ID3D12Resource* const surface_buffer{ (ID3D12Resource* const)dx_surface->GetNativeBuffer(dx_surface->GetCurrentBBIndex()) };
    D3D12_RESOURCE_STATES last_surface_state{};
    DX12Barriers barriers{};
    
    for (const auto& pass : m_Passes) {
        const u32 num_rtvs{ pass.NumRtvs };
        const bool has_dsv{ (bool)pass.HasDsv };

        if (pass.Barriers.Size()) {
            for (u32 i{ 0 }; i < pass.Barriers.Size(); ++i) {
                const BarrierDesc& desc{ pass.Barriers[i] };
                D3D12_RESOURCE_BARRIER b{};
                b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                b.Transition.Subresource = 0;
                b.Transition.StateBefore = desc.Before;
                b.Transition.StateAfter = desc.After;

                if (desc.Resource == (u32)~0) {
                    b.Transition.pResource = surface_buffer;
                    last_surface_state = desc.After;
                }
                else {
                    const Resource& resource{ m_DescResources[desc.Resource] };
                    const u32 actual_index{ CalculateTemporal(frameNumber, resource.Start, resource.Count) };
                    b.Transition.pResource = m_Resources[actual_index];
                    continue;
                }

                barriers.Add(b);
            }

            barriers.Apply(cmd_data.List);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE targets[RHI_MAX_TARGET_COUNT]{};
        D3D12_CPU_DESCRIPTOR_HANDLE depth{};

        for (u16 i{ 0 }; i < num_rtvs; ++i) {
            const u16 view_handle{ pass.Rtvs[i].View };
            if (view_handle == (u16)~0) {
                targets[i] = dx_surface->GetBufferDescriptor(dx_surface->GetCurrentBBIndex());
            }
            else {
                const View& view_desc{ m_DescViews[view_handle] };
                const Resource& resource{ m_DescResources[view_desc.Resource] };

                const u32 actual_index{ CalculateTemporal(frameNumber, view_desc.BaseIndex, resource.Count) };

                targets[i] = m_RtvHeap.GetCpuHandle(actual_index);
            }

            if (pass.HasTargetClear(i)) {
                cmd_data.List->ClearRenderTargetView(targets[i],
                    &pass.RtvClearValues[i].X,
                    0,
                    nullptr);
            }
        }

        if (has_dsv) {
            const u16 view_handle{ pass.Dsv.View };
            const View& view_desc{ m_DescViews[view_handle] };
            const Resource& resource{ m_DescResources[view_desc.Resource] };

            const u32 actual_index{ CalculateTemporal(frameNumber, view_desc.BaseIndex, resource.Count) };

            depth = m_DsvHeap.GetCpuHandle(actual_index);
            if (pass.HasDepthClear()) {
                //TODO: Only depth or stencil???
                cmd_data.List->ClearDepthStencilView(depth,
                    D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                    pass.DepthClearValue.Depth,
                    pass.DepthClearValue.Stencil,
                    0,
                    nullptr);
            }
        }

        cmd_data.List->OMSetRenderTargets(num_rtvs, &targets[0], FALSE, has_dsv ? &depth : nullptr);

        pass.Func(builder);
        Cmd::ParseCommandStream(builder, cmd_data);
        builder.Reset();
    }

    D3D12_RESOURCE_BARRIER present_barrier{};
    present_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    present_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    present_barrier.Transition.pResource = surface_buffer;
    present_barrier.Transition.Subresource = 0;
    present_barrier.Transition.StateBefore = last_surface_state;
    present_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    barriers.Add(present_barrier);
    barriers.Apply(cmd_data.List);

    m_Queue.End(dx_surface);
}

void
CRHIFrameGraph_DX12::WaitIdle() {
    m_Queue.Flush();
}


Vector<Vector<u32>>
CRHIFrameGraph_DX12::GetDependencies(
    const RHIGraphBuilder& builder) const {
    const Vector<RHIGraphBuilder::FGPassDesc>& passes{ builder.GetPasses() };
    const u32 num_passes{ passes.Size() };

    Vector<Vector<u32>> dependencies(num_passes);

    std::unordered_map<FGResource, Vector<u32>> writers;
    writers.reserve(num_passes * 2);

    for (u32 passIndex{ 0 }; passIndex < num_passes; ++passIndex) {
        for (const RHIGraphBuilder::FGResourceUsage& write : passes[passIndex].Writes) {
            writers[write.Resource].PushBack(passIndex);
        }
    }

    for (u32 passIndex{ 0 }; passIndex < num_passes; ++passIndex) {
        auto& deps = dependencies[passIndex];

        for (const RHIGraphBuilder::FGResourceUsage& read : passes[passIndex].Reads) {
            auto it = writers.find(read.Resource);
            if (it == writers.end())
                continue;

            for (u32 writerPass : it->second) {
                if (writerPass != passIndex) {
                    deps.PushBack(writerPass);
                }
            }
        }

        // Optional: remove duplicates
        std::sort(deps.begin(), deps.end());
        deps.Erase(std::unique(deps.begin(), deps.end()), deps.end());
    }

    return dependencies;
}

Vector<u32>
CRHIFrameGraph_DX12::TopologicalSort(
    const Vector<Vector<u32>>& dependencies) const {
    const u32 numPasses = static_cast<u32>(dependencies.Size());

    Vector<u32> inDegree(numPasses, 0);
    Vector<std::vector<u32>> dependents(numPasses);

    for (u32 pass = 0; pass < numPasses; ++pass) {
        for (u32 dep : dependencies[pass]) {
            ++inDegree[pass];
            dependents[dep].push_back(pass);
        }
    }

    std::queue<u32> ready;
    for (u32 i = 0; i < numPasses; ++i) {
        if (inDegree[i] == 0) {
            ready.push(i);
        }
    }

    Vector<u32> sorted;
    sorted.Reserve(numPasses);

    while (!ready.empty()) {
        u32 pass = ready.front();
        ready.pop();

        sorted.PushBack(pass);

        for (u32 dependent : dependents[pass]) {
            if (--inDegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (sorted.Size() != numPasses) {
        // Cycle detected: graph is invalid
        LOG_ERROR("D3D12RHI: Cycle detected in Task Graph!");
    }

    return sorted;
}

void
CRHIFrameGraph_DX12::PrintDependencies(
    const Vector<Vector<u32>>& dependencies) const {
    LOG_DEBUG("Task Graph Dependencies:");

    for (u32 pass = 0; pass < dependencies.Size(); ++pass) {
        LOG_DEBUG("Pass %u depends on: ");

        if (dependencies[pass].Empty()) {
            LOG_DEBUG("(none)");
        }
        else {
            for (u32 dep : dependencies[pass]) {
                LOG_DEBUG("    %u", dep);
            }
        }
    }
}
}
