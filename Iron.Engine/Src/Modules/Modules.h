#pragma once
#include <Iron.Engine/Engine.h>

#include <unordered_map>
#include <string>

namespace Iron {

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
