#include <Iron.FileSystem/FileSystem.h>

namespace Iron::FS {
class CFileSystem : public IFileSystem {
public:
    Result::Code Mount(
        const char* virtualPath,
        const char* actualPath) override;
};

class CFileSystemFactory : public IFileSystemFactory {
public:
};
}

extern "C" __declspec(dllexport)
Iron::Result::Code
GetFactory(Iron::FS::IFileSystemFactory** factory) {
    if (!factory) {
        return Iron::Result::ENullptr;
    }

    using namespace Iron::FS;
    CFileSystemFactory* temp{ new CFileSystemFactory() };
    if (!temp) {
        return Iron::Result::ENomemory;
    }

    *factory = temp;

    return Iron::Result::Ok;
}
