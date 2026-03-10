#pragma once
#include <Iron.Core/Core.h>

#define RHI_TEMPORAL_COUNT_BITS 4
#define RHI_VIEW_INDEX_BITS 12
#define RHI_COMMAND_ALIGN 8
#define RHI_MAX_NAME 128
#define RHI_MAX_SAMPLERS 128
#define RHI_MAX_TEMPORAL 1 << RHI_TEMPORAL_COUNT_BITS
#define RHI_MAX_VIEWS_PER_TYPE (1 << RHI_VIEW_INDEX_BITS) - 1
#define RHI_MAX_TARGET_COUNT 8

#ifndef RHI_ENABLE_STREAM_CHECK
#define RHI_ENABLE_STREAM_CHECK 1
#endif

#if RHI_ENABLE_STREAM_CHECK
#define RHI_VALIDATE_CMD(x) if(!x) { LOG_ERROR("Buffer for commands is full, consider a larger size!"); return; }
#else
#define RHI_VALIDATE_CMD(x) x
#endif

namespace Iron::RHI {
struct RHIBackend {
    enum Value : u32 {
        DirectX11 = 0,
        DirectX12,
    };
};

//Matches DXGI_FORMAT
struct RHIFormat {
    enum Fmt : u32 {
        UNKNOWN,
        R32G32B32A32_TYPELESS,
        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32_TYPELESS,
        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,
        R16G16B16A16_TYPELESS,
        R16G16B16A16_FLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,
        R32G32_TYPELESS,
        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,
        R32G8X24_TYPELESS,
        D32_FLOAT_S8X24_UINT,
        R32_FLOAT_X8X24_TYPELESS,
        X32_TYPELESS_G8X24_UINT,
        R10G10B10A2_TYPELESS,
        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,
        R11G11B10_FLOAT,
        R8G8B8A8_TYPELESS,
        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_UINT,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,
        R16G16_TYPELESS,
        R16G16_FLOAT,
        R16G16_UNORM,
        R16G16_UINT,
        R16G16_SNORM,
        R16G16_SINT,
        R32_TYPELESS,
        D32_FLOAT,
        R32_FLOAT,
        R32_UINT,
        R32_SINT,
        R24G8_TYPELESS,
        D24_UNORM_S8_UINT,
        R24_UNORM_X8_TYPELESS,
        X24_TYPELESS_G8_UINT,
        R8G8_TYPELESS,
        R8G8_UNORM,
        R8G8_UINT,
        R8G8_SNORM,
        R8G8_SINT,
        R16_TYPELESS,
        R16_FLOAT,
        D16_UNORM,
        R16_UNORM,
        R16_UINT,
        R16_SNORM,
        R16_SINT,
        R8_TYPELESS,
        R8_UNORM,
        R8_UINT,
        R8_SNORM,
        R8_SINT,
        A8_UNORM,
        R1_UNORM,
        R9G9B9E5_SHAREDEXP,
        R8G8_B8G8_UNORM,
        G8R8_G8B8_UNORM,
        BC1_TYPELESS,
        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_TYPELESS,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_TYPELESS,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_TYPELESS,
        BC4_UNORM,
        BC4_SNORM,
        BC5_TYPELESS,
        BC5_UNORM,
        BC5_SNORM,
        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        B8G8R8A8_UNORM,
        B8G8R8X8_UNORM,
        R10G10B10_XR_BIAS_A2_UNORM,
        B8G8R8A8_TYPELESS,
        B8G8R8A8_UNORM_SRGB,
        B8G8R8X8_TYPELESS,
        B8G8R8X8_UNORM_SRGB,
        BC6H_TYPELESS,
        BC6H_UF16,
        BC6H_SF16,
        BC7_TYPELESS,
        BC7_UNORM,
        BC7_UNORM_SRGB,
        AYUV,
        Y410,
        Y416,
        NV12,
        P010,
        P016,
        OPAQUE_420,
        YUY2,
        Y210,
        Y216,
        NV11,
        AI44,
        IA44,
        P8,
        A8P8,
        B4G4R4A4_UNORM,
        P208,
        V208,
        V408,
        SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
        SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
        A4B4G4R4_UNORM,
    };
};

struct AdapterType {
    enum Type : u32 {
        Unknown = 0,
        Discrete,
        Integrated,
        Virtual,
    };
};

struct CommandBuilderType {
    enum Type : u8 {
        Graphics = 0,
        Compute,
        Transfer
    };
};

struct ResourceDimension {
    enum Dim : u32 {
        Unknown = 0,
        Buffer,
        Texture1D,
        Texture2D,
        Texture3D,
    };
};

struct ResourceUsage {
    enum Usage : u32 {
        Default = 0,
        Immutable,
        Dynamic,
        Copy,
    };
};

struct ResourceFlags {
    enum Flags : u32 {
        None = 0x00,
        AllowShaderResource = 0x01,
        BindVertexBuffer = 0x02,
        BindIndexBuffer = 0x04,
        TextureCube = 0x08,
        Structured = 0x10,
    };
};

struct ResourceState {
    enum State : u32 {
        Generic = 0,
        VertexBuffer = 0x01,
        IndexBuffer = 0x02,
        ConstantBuffer = 0x04,
        RenderTarget = 0x08,
        UnorderedAccess = 0x10,
        DepthRead = 0x20,
        DepthWrite = 0x40,
        PixelResource = 0x80,
        NonPixelResource = 0x100,
        IndirectArgs = 0x200,
        CopyDest = 0x400,
        CopySrc = 0x800,
        Present = 0x1000,
    };
};

struct ResourceViewType {
    enum Type : u32 {
        ShaderResource = 0,
        RenderTarget,
        DepthStencil,
        UnorderedAccess,
    };
};

struct ShaderType {
    enum Type : u32 {
        All = 0,
        Compute,
        Graphics,
        Vertex,
        Pixel,
    };
};

struct PipelineLayoutParamType {
    enum Type : u32 {
        ShaderResource = 0,
        ConstantBuffer,
        UnorderedAccess,
        Sampler,
    };
};

struct FGResourceType {
    enum Type : u32 {
        Texture = 0,
        Buffer,
        Swapchain
    };
};

struct FGClearOp {
    enum Op : u32 {
        None = 0,
        RenderTarget,
        DepthStencil,
    };
};

struct FGCompileFlags {
    enum Flags : u32 {
        None = 0x00,
        LogInfo = 0x01,
        DebugNames = 0x02,
        Bindless = 0x04,
    };
};

struct VersionData {
    u8                  Major;
    u8                  Minor;
};

struct DeviceInitInfo {
    RHIBackend::Value   Backend;
    bool                Debug;

    //Disabling the GPU timeout is only guaranteed for non-framegraph workloads.
    bool                DisableGPUTimeout;
    u32                 MaxShaderResources;
};

struct DeviceFeatures {
    bool                Bindless : 1;
    bool                PushConstants : 1;
    VersionData         ShaderModel;
    VersionData         FeatureLevel;
};

struct ResourceInitInfo {
    ResourceDimension::Dim  Dimension;
    u32                     Width;
    u32                     Height;
    u32                     DepthOrArray;
    u32                     StructuredStride;
    u32                     MipLevels;
    RHIFormat::Fmt          Format;
    ResourceUsage::Usage    Usage;
    bool                    CPURead;
    bool                    CPUWrite;
    u32                     Flags;
};

struct SurfaceInitInfo {
    void*               Native;
    u32                 Width;
    u32                 Height;
    RHIFormat::Fmt      Format;
    bool                TripleBuffering;
    bool                AllowTearing;
};

struct SubresourceRange {
    u32                             BaseMip{ 0 };
    u32                             MipCount{ 1 };
    u32                             BaseLayer{ 0 };
    u32                             LayerCount{ 1 };
    u32                             Plane{ 0 }; // depth=0, stencil=1, etc.
};

struct DepthStencilClear {
    f32                             Depth;
    u8                              Stencil;
};

struct ClearValue {
    union {
        f32                         Color[4];
        DepthStencilClear           Depth;
    };
};

struct PipelineLayoutParam {
    PipelineLayoutParamType::Type   Type{};
    ShaderType::Type                Visibility{ ShaderType::All };
    u32                             Slot{};
    u32                             Space{ 0 };
    u32                             Count{ 1 };
    bool                            Bindless{ false };

    constexpr inline void AsSRV(u32 slot, u32 space) {
        Type = PipelineLayoutParamType::ShaderResource;
        Slot = slot;
        Space = space;
    }

    constexpr inline void AsCBV(u32 slot, u32 space) {
        Type = PipelineLayoutParamType::ConstantBuffer;
        Slot = slot;
        Space = space;
    }

    constexpr inline void AsUAV(u32 slot, u32 space) {
        Type = PipelineLayoutParamType::UnorderedAccess;
        Slot = slot;
        Space = space;
    }

    constexpr inline void AsSampler(u32 slot, u32 space) {
        Type = PipelineLayoutParamType::Sampler;
        Slot = slot;
        Space = space;
    }

    constexpr inline void AsSRVRange(u32 slot, u32 space, u32 count) {
        Type = PipelineLayoutParamType::ShaderResource;
        Slot = slot;
        Space = space;
        Count = count;
    }

    constexpr inline void AsCBVRange(u32 slot, u32 space, u32 count) {
        Type = PipelineLayoutParamType::ConstantBuffer;
        Slot = slot;
        Space = space;
        Count = count;
    }

    constexpr inline void AsUAVRange(u32 slot, u32 space, u32 count) {
        Type = PipelineLayoutParamType::UnorderedAccess;
        Slot = slot;
        Space = space;
        Count = count;
    }

    constexpr inline void AsSamplerRange(u32 slot, u32 space, u32 count) {
        Type = PipelineLayoutParamType::Sampler;
        Slot = slot;
        Space = space;
        Count = count;
    }
};

struct PipelineLayoutInitInfo {
    u32                             NumParams;
    PipelineLayoutParam*            Params;

    //TODO: Static samplers

    u32                             PushConstantSize;
};

struct ComputePipelineInitInfo {
};

struct FGResourceInitInfo {
    const char*                     Name;
    FGResourceType::Type            Type{};
    u32                             Width{};
    u32                             Height{};
    u32                             DepthOrArray{ 1 };
    u32                             MipLevels{ 1 };
    RHIFormat::Fmt                  StorageFormat{}; //MUST be typeless
    u32                             TemporalCount{ 1 };


    constexpr FGResourceInitInfo(
        const char*                     name,
        FGResourceType::Type            type,
        u32                             width,
        u32                             height,
        RHIFormat::Fmt                  storageFormat,
        u32                             depthOrArray = 1,
        u32                             mipLevels = 1,
        u32                             temporal = 1) {
        Name = StrDup(name);
        Type = type;
        Width = width;
        Height = height;
        DepthOrArray = depthOrArray;
        MipLevels = mipLevels;
        StorageFormat = storageFormat;
        TemporalCount = temporal;
    }

    constexpr u8 GetFlags() const {
        return Flag_Field;
    }

    constexpr ResourceState::State GetLastState() const {
        return Last_State;
    }

private:
    friend class RHIGraphBuilder;
    u8                              Flag_Field{};
    ResourceState::State            Last_State{};
};

constexpr static bool
FmtIsTypeless(RHIFormat::Fmt fmt) {
    return fmt == RHIFormat::B8G8R8A8_TYPELESS
        || fmt == RHIFormat::B8G8R8X8_TYPELESS
        || fmt == RHIFormat::BC1_TYPELESS
        || fmt == RHIFormat::BC2_TYPELESS
        || fmt == RHIFormat::BC3_TYPELESS
        || fmt == RHIFormat::BC4_TYPELESS
        || fmt == RHIFormat::BC5_TYPELESS
        || fmt == RHIFormat::BC6H_TYPELESS
        || fmt == RHIFormat::BC7_TYPELESS
        || fmt == RHIFormat::R10G10B10A2_TYPELESS
        || fmt == RHIFormat::R16G16B16A16_TYPELESS
        || fmt == RHIFormat::R16G16_TYPELESS
        || fmt == RHIFormat::R16_TYPELESS
        || fmt == RHIFormat::R24G8_TYPELESS
        || fmt == RHIFormat::R24_UNORM_X8_TYPELESS
        || fmt == RHIFormat::R32G32B32A32_TYPELESS
        || fmt == RHIFormat::R32G32B32_TYPELESS
        || fmt == RHIFormat::R32G32_TYPELESS
        || fmt == RHIFormat::R32G8X24_TYPELESS
        || fmt == RHIFormat::R32_FLOAT_X8X24_TYPELESS
        || fmt == RHIFormat::R32_TYPELESS
        || fmt == RHIFormat::R8G8B8A8_TYPELESS
        || fmt == RHIFormat::R8G8_TYPELESS
        || fmt == RHIFormat::R8_TYPELESS
        || fmt == RHIFormat::X24_TYPELESS_G8_UINT
        || fmt == RHIFormat::X32_TYPELESS_G8X24_UINT;
}

constexpr static bool
FmtIsDepth(RHIFormat::Fmt fmt) {
    return fmt == RHIFormat::D16_UNORM
        || fmt == RHIFormat::D24_UNORM_S8_UINT
        || fmt == RHIFormat::D32_FLOAT
        || fmt == RHIFormat::D32_FLOAT_S8X24_UINT;
}

typedef u32 FGResource;
class RHICommandBuilder;
class RHIGraphBuilder;
class IRHIAdapter;
class IRHIDevice;
class IRHIResource;
class IRHIPipelineLayout;
class IRHIPipeline;
class IRHISurface;
class IRHIFrameGraph;

typedef void(*FGPassFunc)(RHICommandBuilder&);

class IRHIFactory : public IObjectBase {
public:
    virtual ~IRHIFactory() = default;

    virtual u32 GetAdapterCount() const = 0;
    virtual void GetAdapters(
        IRHIAdapter** adapters,
        u32 count) const = 0;

    virtual Result::Code CreateDevice(
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info,
        IRHIDevice** outHandle) = 0;

    virtual bool SupportsTearing() const = 0;
    virtual bool IsDebug() const = 0;

    virtual void* const GetNative() const = 0;
};

class IRHIAdapter : public IObjectBase {
public:
    virtual ~IRHIAdapter() = default;

    virtual const char* GetName() const = 0;
    virtual AdapterType::Type GetType() const = 0;
    virtual u32 GetVendorID() const = 0;
    virtual u32 GetDeviceID() const = 0;
    virtual void* const GetNative() const = 0;
};

class IRHIDevice : public IObjectBase {
public:
    virtual ~IRHIDevice() = default;

    virtual Result::Code CreateResource(
        const ResourceInitInfo& info,
        IRHIResource** resource) = 0;

    virtual Result::Code CreatePipelineLayout(
        const PipelineLayoutInitInfo& info,
        IRHIPipelineLayout** outHandle) = 0;

    //virtual Result::Code CreateComputePipeline(
    //    const ComputePipelineInitInfo& info,
    //    IRHIPipelineLayout** outHandle) = 0;

    virtual Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) = 0;

    virtual Result::Code CreateFrameGraph(
        const RHIGraphBuilder& builder,
        u32 flags,
        IRHIFrameGraph** outHandle) = 0;


    virtual void GetFeatures(
        DeviceFeatures* features) = 0;

    virtual void* const GetNative() const = 0;
};

class IRHIResource : public IObjectBase {
public:
    virtual ~IRHIResource() = default;

    virtual void* const GetNative() const = 0;
};

class IRHIPipelineLayout : public IObjectBase {
public:
    virtual ~IRHIPipelineLayout() = default;

    virtual void* const GetNative() const = 0;
};

class IRHIPipeline : public IObjectBase {
public:
    virtual ~IRHIPipeline() = default;

    virtual void* const GetNative() const = 0;
};

class IRHISurface : public IObjectBase {
public:
    virtual ~IRHISurface() = default;

    virtual u32 GetBufferCount() const = 0;
    virtual void* const GetNative() const = 0;
    virtual void* const GetNativeBuffer(u32 index) const = 0;
};

class IRHIFrameGraph : public IObjectBase {
public:
    virtual ~IRHIFrameGraph() = default;

    virtual void Execute(
        IRHISurface* const surface,
        u64 frameNumber) = 0;

    virtual void WaitIdle() = 0;
};

//TODO: Rework for better clarity
class RHIGraphBuilder {
    static constexpr u32 InvalidPass = ~0u;
public:
    constexpr static u8 ResourceFlag_SRV{ 0x01 };
    constexpr static u8 ResourceFlag_RTV{ 0x02 };
    constexpr static u8 ResourceFlag_DSV{ 0x04 };
    constexpr static u8 ResourceFlag_UAV{ 0x08 };

    struct FGResourceUsage
    {
        FGResource                      Resource;
        ResourceState::State            State;
        RHIFormat::Fmt                  ViewFormat;
        SubresourceRange                Range;
        u32                             Slot;
        FGClearOp::Op                   ClearOp{ FGClearOp::None };
        ClearValue                      ClearValue;
    };

    struct FGPassDesc
    {
        const char*                     Name{};
        FGPassFunc                      Func{};
        Vector<FGResourceUsage>         Reads{};
        Vector<FGResourceUsage>         Writes{};
    };

public:
    FGResource RegisterOutput(
        const char* name,
        u32 width,
        u32 height,
        RHIFormat::Fmt format,
        bool tripleBuffering) {
        FGResourceInitInfo desc(
            name,
            FGResourceType::Swapchain,
            width,
            height,
            format,
            1,
            1,
            tripleBuffering ? 3 : 2
        );

        FGResource handle{ (FGResource)m_Resources.Size() };
        m_Resources.PushBack(desc);
        return handle;
    }

    FGResource CreateResource(
        const FGResourceInitInfo& desc) {
        if (!FmtIsTypeless(desc.StorageFormat)) {
            LOG_ERROR("RHI: Task Graph base resource MUST be typeless!");
            return (FGResource)~0;
        }

        FGResource handle{ (FGResource)m_Resources.Size() };

        m_Resources.PushBack(desc);
        return handle;
    }

    u32 BeginPass(
        const char* name,
        FGPassFunc func) {
        if (m_BuildingPass) {
            LOG_ERROR("RHI: Nested passes are not allowed");
        }
        if (!func) {
            LOG_ERROR("RHI: Nullptr func passed to BeginPass()!");
        }

        u32 handle{ m_Passes.Size() };

        FGPassDesc pass{};
        pass.Name = StrDup(name);
        pass.Func = func;

        m_Passes.PushBack(pass);
        m_CurrentPass = handle;
        m_BuildingPass = true;

        return handle;
    }

    void EndPass() {
        ValidatePass();
        m_BuildingPass = false;
        m_CurrentPass = InvalidPass;
    }

    void AddDepth(FGResource    resource,
        RHIFormat::Fmt          viewFormat,
        u32                     state,
        SubresourceRange        range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, 0 };
        if (state & ResourceState::DepthRead) {
            m_Passes[m_CurrentPass].Reads.PushBack(usage);
        }
        else {
            m_Passes[m_CurrentPass].Writes.PushBack(usage);
        }
    }

    void Read(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, slot };
        m_Passes[m_CurrentPass].Reads.PushBack(usage);
    }

    void Write(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, slot };
        m_Passes[m_CurrentPass].Writes.PushBack(usage);
    }

    void WriteClear(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        ClearValue          clear,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, slot,
        FmtIsDepth(viewFormat)
                        ? FGClearOp::DepthStencil
                        : FGClearOp::RenderTarget,
            clear };
        m_Passes[m_CurrentPass].Writes.PushBack(usage);
    }

    void ReadWrite(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        m_Passes[m_CurrentPass].Writes.PushBack(
            { resource, (ResourceState::State)state, viewFormat, range, slot });
    }

    const Vector<FGPassDesc>& GetPasses() const {
        return m_Passes;
    }

    const Vector<FGResourceInitInfo>& GetResources() const {
        return m_Resources;
    }

    const u32 GetSRViewCounter() const { return m_NumSrvs; }
    const u32 GetRTViewCounter() const { return m_NumRtvs; }
    const u32 GetDSViewCounter() const { return m_NumDsvs; }
    const u32 GetUAViewCounter() const { return m_NumUavs; }

private:
    void AddFlags(FGResource resource, ResourceState::State state) {
        FGResourceInitInfo& info{ m_Resources[resource] };

        info.Last_State = state;

        if (state & ResourceState::RenderTarget) {
            info.Flag_Field |= ResourceFlag_RTV;
            m_NumRtvs += info.TemporalCount;
        }
        if (state & ResourceState::DepthRead
            || state & ResourceState::DepthWrite) {
            info.Flag_Field |= ResourceFlag_DSV;
            m_NumDsvs += info.TemporalCount;
        }
        if (state & ResourceState::PixelResource
            || state & ResourceState::NonPixelResource) {
            info.Flag_Field |= ResourceFlag_SRV;
            m_NumSrvs += info.TemporalCount;
        }
        if (state & ResourceState::UnorderedAccess) {
            info.Flag_Field |= ResourceFlag_UAV;
            m_NumUavs += info.TemporalCount;
        }
    }

    void ValidatePass() const {
        if (!m_BuildingPass) {
            LOG_ERROR("RHI: No active pass!");
        }
        if (m_CurrentPass == InvalidPass) {
            LOG_ERROR("RHI: Invalid pass!");
        }
    }

    void Reset() {
        m_Passes.Clear();
        m_Resources.Clear();
        m_CurrentPass = InvalidPass;
        m_BuildingPass = false;
    }


    Vector<FGPassDesc>          m_Passes{};
    Vector<FGResourceInitInfo>  m_Resources{};

    u32                         m_NumSrvs{};
    u32                         m_NumRtvs{};
    u32                         m_NumDsvs{};
    u32                         m_NumUavs{};
    u32                         m_CurrentPass{ InvalidPass };
    bool                        m_BuildingPass{ false };
};

//TODO: Remove PayloadSize from CmdHeader
class RHICommandBuilder {
public:
    struct CommandId {
        enum Id : u32 {
            CopyResource,
            SetGraphicsLayout,
            SetComputeLayout,
            Draw,
            DrawInstanced,
            DrawIndexed,
            DrawIndexedInstanced,
            Count
        };
    };

    struct CmdHeader
    {
        u32         Id;
        u32         PayloadSize;
    };

    struct CmdCopyResourceInfo {
        IRHIResource*           Src;
        IRHIResource*           Dst;
    };

    struct CmdSetGraphicsLayoutInfo {
        IRHIPipelineLayout*     Layout;
    };

    struct CmdSetComputeLayoutInfo {
        IRHIPipelineLayout*     Layout;
    };

    struct CmdDrawInfo {
        u32                     VertexCount;
        u32                     BaseVertex;
    };

    struct CmdDrawInstancedInfo {
        u32                     VertexCount;
        u32                     InstanceCount;
        u32                     BaseVertex;
        u32                     BaseInstance;
    };

    struct CmdDrawIndexedInfo {
        u32                     IndexCount;
        u32                     BaseIndex;
        u32                     BaseVertex;
    };

    struct CmdDrawInstancedIndexedInfo {
        u32                     IndexCount;
        u32                     InstanceCount;
        u32                     BaseIndex;
        u32                     BaseVertex;
        u32                     BaseInstance;
    };

    static_assert(sizeof(CmdHeader) == RHI_COMMAND_ALIGN, "Invalid command align");

public:
    RHICommandBuilder(u32 capacity)
        :
        m_Stream(),
        m_Offset(),
        m_Capacity(capacity) {
        m_Stream = (u8*)MemAlloc(capacity);
        if (!m_Stream) {
            LOG_ERROR("No command stream memory!");
        }
    }

    RHICommandBuilder(const RHICommandBuilder&) = delete;
    RHICommandBuilder& operator=(const RHICommandBuilder&) = delete;

    ~RHICommandBuilder() {
        MemFree(m_Stream);
    }

    template<typename T>
    T* Allocate(u32 id) {
        constexpr u32 aligned_payload_size{ Math::AlignUp((u32)sizeof(T), (u32)RHI_COMMAND_ALIGN) };
        const u32 total{ sizeof(CmdHeader) + aligned_payload_size };

#if RHI_ENABLE_STREAM_CHECK
        if (!m_Stream || m_Offset + total > m_Capacity) {
            LOG_ERROR("No command stream memory!");
            return nullptr;
        }
#endif

        auto* header = (CmdHeader*)(m_Stream + m_Offset);
        header->Id = id;
        header->PayloadSize = aligned_payload_size;

        T* payload = (T*)(header + 1);

        m_Offset += total;

        return payload;
    }

    void Reset() { m_Offset = 0; }

    constexpr const u8* const GetStream() const {
        return m_Stream;
    }

    constexpr u32 GetOffset() const {
        return m_Offset;
    }

    constexpr u32 GetCapacity() const {
        return m_Capacity;
    }

public:
    inline void CopyResource(IRHIResource* Src,
        IRHIResource* Dst) {
        auto* cmd{ Allocate<CmdCopyResourceInfo>(CommandId::CopyResource) };
        RHI_VALIDATE_CMD(cmd);
        cmd->Src = Src;
        cmd->Dst = Dst;
    }

    inline void SetGraphicsLayout(IRHIPipelineLayout* layout) {
        auto* cmd{ Allocate<CmdSetGraphicsLayoutInfo>(CommandId::SetGraphicsLayout) };
        RHI_VALIDATE_CMD(cmd);
        cmd->Layout = layout;
    }

    inline void SetComputeLayout(IRHIPipelineLayout* layout) {
        auto* cmd{ Allocate<CmdSetComputeLayoutInfo>(CommandId::SetComputeLayout) };
        RHI_VALIDATE_CMD(cmd);
        cmd->Layout = layout;
    }

    inline void Draw(u32 VertexCount,
        u32 BaseVertex) {
        auto* cmd{ Allocate<CmdDrawInfo>(CommandId::Draw) };
        RHI_VALIDATE_CMD(cmd);
        cmd->VertexCount = VertexCount;
        cmd->BaseVertex = BaseVertex;
    }

    inline void DrawInstanced(u32 VertexCount,
        u32 InstanceCount,
        u32 BaseVertex,
        u32 BaseInstance) {
        auto* cmd{ Allocate<CmdDrawInstancedInfo>(CommandId::DrawInstanced) };
        RHI_VALIDATE_CMD(cmd);
        cmd->VertexCount = VertexCount;
        cmd->InstanceCount = InstanceCount;
        cmd->BaseVertex = BaseVertex;
        cmd->BaseInstance = BaseInstance;
    }

    inline void DrawIndexed(u32 IndexCount,
        u32 BaseIndex,
        u32 BaseVertex) {
        auto* cmd{ Allocate<CmdDrawIndexedInfo>(CommandId::DrawIndexed) };
        RHI_VALIDATE_CMD(cmd);
        cmd->IndexCount = IndexCount;
        cmd->BaseIndex = BaseIndex;
        cmd->BaseVertex = BaseVertex;
    }

    inline void DrawIndexedInstanced(u32 IndexCount,
        u32 InstanceCount,
        u32 BaseIndex,
        u32 BaseVertex,
        u32 BaseInstance) {
        auto* cmd{ Allocate<CmdDrawInstancedIndexedInfo>(CommandId::DrawIndexedInstanced) };
        RHI_VALIDATE_CMD(cmd);
        cmd->IndexCount = IndexCount;
        cmd->InstanceCount = InstanceCount;
        cmd->BaseIndex = BaseIndex;
        cmd->BaseVertex = BaseVertex;
        cmd->BaseInstance = BaseInstance;
    }

private:
    u8*         m_Stream;
    u32         m_Offset;
    u32         m_Capacity;
};
}
