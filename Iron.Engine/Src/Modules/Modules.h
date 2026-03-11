#pragma once
#include <Iron.Engine/Engine.h>

#include <unordered_map>
#include <string>

namespace Iron {
struct EngineModule {
    u64             Id;
    void*           Library;
    IObjectBase*    Factory;

    constexpr static inline u64 Hash(const char* Name) noexcept {
        u64 Hash{ 0x468a3276cf9809ed };

        while (*Name != '\0') {
            Hash ^= (u64)*Name + 0x83987239870acefd;
            ++Name;
        }

        return Hash;
    }

    constexpr static inline bool IsValid(const EngineModule& M) {
        return M.Library && M.Factory;
    }

    constexpr static inline bool IsLoaded(const EngineModule& M) {
        return M.Library;
    }
};

class ModuleManager {
public:
    ~ModuleManager();

    Result::Code LoadModule(const char* Path, u64 Id);
    void UnloadModule(u64 Id);

    IObjectBase* const GetFactory(u64 Id) const;

    void Reset();

private:
    std::unordered_map<u64, EngineModule> m_Modules{};
};
}
