#include <Iron.Core/Core.h>

namespace Iron {
typedef Id::TypeId EntityId;

struct STransform {
    Math::V3        Position;
    Math::V3        Rotation;
    Math::V3        Scale;
}

struct SComponentInfo {
    u32            Size : 12{};
    u32            Alignment : 4{};
    u32            Id : 16{};
}

class CComponentChunk {
public:
    constexpr static u32 ChunkSize{ 16 * 1024 };

    CComponentChunk(TArray<SComponentInfo>& components);

private:
    u8*         m_Data{ nullptr };
    TArray<u8*> m_ComponentPointers{};
    EntityId*   m_Entities{};
    u32         m_MaxEntities{};
};

class CComponentArchetype {
public:

private:
    TArray<SComponentInfo>  m_ComponentInfos{};
    Vector<CComponentChunk> m_Chunks{};
};

class CComponentSystem {
public:
    template<typename T>
    void RegisterComponent() {
        static bool seen{ false };
        if(seen) {
            return;
        }

        SComponentInfo info{};
        info.size = sizeof(T);
        info.Alignment = alignof(T);
        info.id = m_ComponentIdCounter++;

        m_ComponentInfos.EmplaceBack(info);
        seen = true;
    }

    EntityId CreateEntity(STransform initialTransform);
    void DestroyEntity(EntityId id);

    SComponentInfo GetComponentInfo(u32 id);

private:
    u32                     m_ComponentIdCounter{ 0 };
    Vector<SComponentInfo>  m_ComponentInfos{};
};
}
