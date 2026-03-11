#pragma once
#include <Iron.RHI/RHI.h>

#include <dxgi1_6.h>

namespace Iron::RHI::Shared {
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
}
