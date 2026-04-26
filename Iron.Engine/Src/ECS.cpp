#include <Iron.Engine/ECS.h>

namespace Iron {
namespace {
u32
CalculateMaxEntities(TArray<SComponentInfo>& components) {
    u32 per_entity{ sizeof(EntityId) };
    for(auto& c : components) {
        per_entity += Math::AlignUp(c.size, c.alignment);
    }

    return CComponentChunk::ChunkSize / per_entity;
}
}//anonymous namespace

CComponentChunk::CComponentChunk(TArray<SComponentInfo>& components) {
    m_MaxEntities = CalculateMaxEntities(components);
    m_Data = MemAlloc(ChunkSize);
    m_Entities = m_Data;
}

EntityId
CComponentSystem::CreateEntity(STransform initialTransform) {

}

void
CComponentSystem::DestroyEntity(EntityId id) {

}

SComponentInfo
CComponentSystem::GetComponentInfo(u32 id) {
    if(id >= m_ComponentInfos.Size())
        return {};

    return m_ComponentInfos[id];
}
}
