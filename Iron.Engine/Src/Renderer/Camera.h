#pragma once
#include <Iron.Core/Core.h>

namespace Iron {
struct Camera {
    Math::V3    Position{ 0,0,0 };
    f32         Pitch{ 0 };
    f32         Yaw{ 0 };
    f32         Fov{ 0.75f * Math::HalfPI };
    f32         Aspect{ 1 };
    f32         Near{ 0.1f };
    f32         Far{ 1000 };

    Math::M4    View{};
    Math::M4    Proj{};
    Math::M4    ViewProj{};

    inline Math::V3 Forward() noexcept {
        return Math::Normalize({
            Math::CosF(Pitch) * Math::SinF(Yaw),
            Math::SinF(Pitch),
            Math::CosF(Pitch) * Math::CosF(Yaw)
            });
    }

    void Update() {
        using namespace Math;
        V3 F{ Forward() };
        View = LookAtLH(Position, Position + F, { 0,1,0 });
        Proj = PerspectiveLH(Fov, Aspect, Near, Far);
        ViewProj = View * Proj;

    }
};
}
