#pragma once
#include <Iron.Core/Core.h>

#define RHI_MAX_NAME 128

#ifndef RHI_ENABLE_STREAM_CHECK
#define RHI_ENABLE_STREAM_CHECK 1
#endif

namespace Iron::RHI {
typedef u32 FGResource;

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

struct CommandBuilderType
{
    enum Type : u8 {
        Graphics = 0,
        Compute,
        Transfer
    };
};

struct DeviceInitInfo {
    RHIBackend::Value   Backend;
    bool                Debug;
    bool                DisableGPUTimeout;
};

struct SurfaceInitInfo {
    void*               Native;
    u32                 Width;
    u32                 Height;
    RHIFormat::Fmt      Format;
    bool                TripleBuffering;
    bool                AllowTearing;
};

struct FGResourceInitInfo {
};

class IRHIAdapter;
class IRHIDevice;
class IRHISurface;

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

    virtual Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) = 0;

    virtual void* const GetNative() const = 0;
};

class IRHISurface : public IObjectBase {
public:
    virtual ~IRHISurface() = default;

    virtual u32 GetBufferCount() const = 0;
    virtual void* const GetNative() const = 0;
};

class IRHIFrameGraph : public IObjectBase {
public:
    virtual ~IRHIFrameGraph() = default;
};

class RHIGraphBuilder {
public:
    FGResource CreateResource() {

    }

private:
};

class RHICommandBuilder {
public:
    struct CommandId {
        enum Id : u16 {
            Draw,
            DrawInstanced,
            DrawIndexed,
            DrawIndexedInstanced,
        };
    };

    struct CmdHeader
    {
        u16 Id;
        u16 Size;
    };

    struct CmdDrawInfo {
        u32         VertexCount;
        u32         BaseVertex;
    };

    struct CmdDrawInstancedInfo {
        u32         VertexCount;
        u32         InstanceCount;
        u32         BaseVertex;
        u32         BaseInstance;
    };

    struct CmdDrawIndexedInfo {
        u32         IndexCount;
        u32         BaseIndex;
        u32         BaseVertex;
    };

    struct CmdDrawInstancedIndexedInfo {
        u32         IndexCount;
        u32         InstanceCount;
        u32         BaseIndex;
        u32         BaseVertex;
        u32         BaseInstance;
    };

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
    bool Allocate(u16 id, const T& cmd)
    {
        constexpr u32 headerAlign{ alignof(CmdHeader) };
        constexpr u32 payloadAlign{ alignof(T) };

        const u32 alignedOffset{ Math::AlignUp(m_Offset, headerAlign) };

#if RHI_ENABLE_STREAM_CHECK
        const u32 total{
            (alignedOffset - m_Offset) +
            sizeof(CmdHeader) +
            Math::AlignUp(sizeof(T), payloadAlign) };

        if (!m_Stream || alignedOffset + total > m_Capacity)
        {
            LOG_ERROR("No command stream memory!");
            return false;
        }
#endif

        m_Offset = alignedOffset;

        CmdHeader header{ id, sizeof(T) };
        MemCopy(m_Stream + m_Offset, &header, sizeof(header));
        m_Offset += sizeof(header);

        m_Offset = Math::AlignUp(m_Offset, payloadAlign);

        MemCopy(m_Stream + m_Offset, &cmd, sizeof(T));
        m_Offset += sizeof(T);

        return true;
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

private:
    u8*         m_Stream;
    u32         m_Offset;
    u32         m_Capacity;
};

class RHITransferCommandList : public RHICommandBuilder {
public:

};

class RHIComputeCommandList : public RHITransferCommandList {
public:

};

class RHIGraphicsCmdList : public RHIComputeCommandList {
public:
    void Draw(u32 VertexCount,
        u32 BaseVertex) {
        Allocate(CommandId::Draw, CmdDrawInfo{
            VertexCount,
            BaseVertex
            });
    }

    void DrawInstanced(u32 VertexCount,
        u32 InstanceCount,
        u32 BaseVertex,
        u32 BaseInstance) {
        Allocate(CommandId::DrawInstanced, CmdDrawInstancedInfo{
            VertexCount,
            InstanceCount,
            BaseVertex,
            BaseInstance
            });
    }

    void DrawIndexed(u32 IndexCount,
        u32 BaseIndex,
        u32 BaseVertex) {
        Allocate(CommandId::DrawIndexed, CmdDrawIndexedInfo{
            IndexCount,
            BaseIndex,
            BaseVertex
            });
    }

    void DrawIndexedInstanced(u32 IndexCount,
        u32 InstanceCount,
        u32 BaseIndex,
        u32 BaseVertex,
        u32 BaseInstance) {
        Allocate(CommandId::DrawIndexedInstanced, CmdDrawInstancedIndexedInfo{
            IndexCount,
            InstanceCount,
            BaseIndex,
            BaseVertex,
            BaseInstance
            });
    }
};
}
