#pragma once
#include <Iron.Core/Core.h>

namespace Iron::AssetCompiler {
struct ShaderType {
    enum Type : u32 {
        Unknown = 0,
        Vertex,
        Pixel,
        Compute,
        Mesh,
        Task,
        Count,
    };
};

struct ShaderFlags {
    enum Flags : u32 {
        None = 0x00,
        Debug = 0x01,
        Allow16BitTypes = 0x02,
        WarningsAreErrors = 0x04,
    };
};

struct ShaderDefine {
    const char*             Name;
    const char*             Definition;
};

struct CompileShaderInfo {
    ShaderType::Type        Type;
    const char*             Entry;
    u32                     Flags;
    ShaderDefine*           Defines;
    u32                     NumDefines;
};

class IShader;
class IShaderCompiler;

class ICompilerFactory : public IObjectBase {
public:
    virtual ~ICompilerFactory() = default;

    virtual Result::Code CreateShaderCompiler(
        Version shaderVersion,
        IShaderCompiler** outHandle) = 0;
};

class IImportedAsset : public IObjectBase {
public:
    virtual ~IImportedAsset() = default;

    virtual Result::Code SaveToFile(
        const char* filePath) = 0;
};

class IShader : public IImportedAsset {
public:
    virtual ~IShader() = default;

    virtual u8* const GetData() = 0;
    virtual u64 GetSize() = 0;
};

class IShaderCompiler : public IObjectBase {
public:
    virtual ~IShaderCompiler() = default;

    virtual Result::Code CompileShaderFromFile(
        const char* fileName,
        const CompileShaderInfo& info,
        IShader** outHandle) = 0;
};
}
