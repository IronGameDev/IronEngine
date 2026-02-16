#include <Iron.Engine/Engine.h>

#include <Windows.h>
#include "Modules.h"

namespace Iron {
typedef Result::Code(*FuncGetVTable)(IObjectBase**);

ModuleManager::~ModuleManager() {
    Reset();
}

Result::Code
ModuleManager::LoadModule(const char* Path, u64 Id) {
    auto Pair{ m_Modules.find(Id) };
    if (Pair != m_Modules.end()) {
        return Result::Ok;
    }

    EngineModule Mod{};
    Mod.Id = Id;
    Mod.Library = LoadLibraryExA(Path, 0, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!Mod.Library) {
        return Result::ELoadlibrary;
    }

    FuncGetVTable Func{ (FuncGetVTable)GetProcAddress((HMODULE)Mod.Library, "GetFactory") };
    if (!Func) {
        FreeLibrary((HMODULE)Mod.Library);
        return Result::EGetvtable;
    }

    Result::Code Res{ Result::Ok };
    Res = Func(&Mod.Factory);
    if (Result::Fail(Res)) {
        FreeLibrary((HMODULE)Mod.Library);
        return Res;
    }

    LOG_INFO("Loaded module %s Id=%ull", Path, Id);
    m_Modules[Id] = Mod;

    return Res;
}

void ModuleManager::UnloadModule(u64 Id) {
    auto It{ m_Modules.find(Id) };
    if (It == m_Modules.end())
        return;

    EngineModule& Mod{ It->second };
    if (EngineModule::IsLoaded(Mod)) {
        SafeRelease(Mod.Factory);
        FreeLibrary((HMODULE)Mod.Library);
    }

    m_Modules.erase(It);
}

IObjectBase* const
ModuleManager::GetFactory(u64 Id) const
{
    auto Pair{ m_Modules.find(Id) };
    if (Pair != m_Modules.end()) {
        return Pair->second.Factory;
    }

    return nullptr;
}

void
ModuleManager::Reset() {
    for (auto It{ m_Modules.begin() }; It != m_Modules.end(); ) {
        if (EngineModule::IsLoaded(It->second)) {
            LOG_WARNING("Dll Id=%ull was not unloaded!", It->second.Id);
            SafeRelease(It->second.Factory);
            FreeLibrary((HMODULE)It->second.Library);

            It = m_Modules.erase(It);
        }
        else {
            ++It;
        }
    }
}
}
