#pragma once
#include <Iron.RHI/RHI.h>

#include <dxgi1_6.h>
#include <d3dcommon.h>

#define LOG_HR(hr) LOG_ERROR(Iron::RHI::Shared::HrToString(hr));

namespace Iron::RHI::Shared {
static const char* HrToString(HRESULT hr) {
    static char buffer[512];

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer,
        sizeof(buffer),
        nullptr
    );

    return buffer;
}

static u64
HashBytes(const void* data, size_t size, u64 hash = 1469598103934665603ull) {
    if (!data) return hash;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

template<typename T>
static u64
HashStruct(const T& s, u64 hash) {
    return HashBytes(&s, sizeof(T), hash);
}

static u64
HashShaderBytecode(const RHIBlob& bc, u64 hash) {
    hash = HashBytes(&bc.Size, sizeof(bc.Size), hash);
    if (bc.Blob && bc.Size > 0) {
        hash = HashBytes(bc.Blob, bc.Size, hash);
    }
    return hash;
}

struct ViewCacheEntry {
    u32                 Resource;
    RHIFormat::Fmt      Format;
    u32                 BaseMip;
    u32                 MipCount;
    u32                 BaseLayer;
    u32                 LayerCount;
    u32                 Plane;
};

struct ViewCacheHasher
{
    std::size_t operator()(const ViewCacheEntry& e) const noexcept {
        std::size_t h = 0;

        auto hash_combine = [](std::size_t& seed, std::size_t v) {
                // 64-bit golden ratio constant
                seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            };

        hash_combine(h, std::hash<u32>{}(e.Resource));
        hash_combine(h, std::hash<u32>{}(static_cast<u32>(e.Format)));
        hash_combine(h, std::hash<u32>{}(e.BaseMip));
        hash_combine(h, std::hash<u32>{}(e.MipCount));
        hash_combine(h, std::hash<u32>{}(e.BaseLayer));
        hash_combine(h, std::hash<u32>{}(e.LayerCount));
        hash_combine(h, std::hash<u32>{}(e.Plane));

        return h;
    }
};

inline bool operator==(const ViewCacheEntry& a, const ViewCacheEntry& b) noexcept {
    return a.Resource == b.Resource &&
        a.Format == b.Format &&
        a.BaseMip == b.BaseMip &&
        a.MipCount == b.MipCount &&
        a.BaseLayer == b.BaseLayer &&
        a.LayerCount == b.LayerCount &&
        a.Plane == b.Plane;
}

constexpr static DXGI_FORMAT
ConvertFormat(RHIFormat::Fmt fmt) {
	return (DXGI_FORMAT)fmt;
}

constexpr static D3D_PRIMITIVE_TOPOLOGY
ConvertTopology(PrimitiveTopology::Val val) {
    switch (val)
    {
    case PrimitiveTopology::Unknown:
        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    case PrimitiveTopology::PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveTopology::LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveTopology::LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveTopology::TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveTopology::TriangleFan:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
    default:
        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}
}
