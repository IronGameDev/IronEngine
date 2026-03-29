#pragma once
#include <Iron.Core/Core.h>

namespace Iron::FS {
struct AssetType {
    enum Type : u32 {
        Unknown = 0,
        Shader,
        Texture,
        Mesh,
    };
};

class IAsset {
public:
    virtual ~IAsset() = 0;

    virtual u64 GetID() = 0;
    virtual AssetType::Type GetType() = 0;
    virtual u32 GetNumDependencies() = 0;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = 0;

    virtual Result::Code Mount(
        const char* virtualPath,
        const char* actualPath) = 0;
};

class IFileSystemFactory {
public:
    virtual ~IFileSystemFactory() = 0;

    virtual Result::Code CreateFileSystem(
        IFileSystem** outHandle) = 0;
};
}
