#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.AssetCompiler/AssetCompiler.h>

#include <string>

namespace Iron::Editor::Assets {
struct AssetMetaData {
    std::string                     Name;
    std::string                     File;
    AssetCompiler::AssetType::Type  Type;
};

class AssetRegistry {
public:
    Result::Code Load(const char* path);

private:
    u64             m_LastPackTime{};
};

extern AssetRegistry g_AssetRegistry;
}
