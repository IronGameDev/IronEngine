#include <Iron.Core/Core.h>

namespace Iron {
struct SComponentInfo {
    u32            Size : 16{};
    u32            Id : 16{};
}

class CComponentChunk {
public:
    constexpr static u32 ChunkSize{ 16 * 1024 };

private:
    u8*         m_Data{ nullptr };
};

class CComponentArchetype{
public:

private:
};

class CComponentSystem {
public:

private:
};
}
