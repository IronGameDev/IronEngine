#include <Iron.AssetCompiler/AssetCompiler.h>
#include <Iron.AssetCompiler/Src/Shaders.h>

#include <filesystem>

namespace Iron::AssetCompiler {
namespace {
class CCompilerFactory : public ICompilerFactory {
public:
    void Release() override {}

    Result::Code CreateShaderCompiler(
        Version shaderVersion,
        IShaderCompiler** outHandle) override;
};

Result::Code
CCompilerFactory::CreateShaderCompiler(
    Version shaderVersion,
    IShaderCompiler** outHandle) {
    if (shaderVersion.Major <= 5) {
        CShaderCompilerD3D* ptr{ new CShaderCompilerD3D(shaderVersion) };
        if (!ptr) {
            return Result::ENomemory;
        }

        *outHandle = ptr;
    }
    else {
        CShaderCompilerDXC* ptr{ new CShaderCompilerDXC(shaderVersion) };
        if (!ptr) {
            return Result::ENomemory;
        }

        *outHandle = ptr;
    }

    return Result::Ok;
}
}//anonymous namespace
}

extern "C" __declspec(dllexport)
Iron::Result::Code
GetFactory(Iron::AssetCompiler::ICompilerFactory** factory) {
    if (!factory) {
        return Iron::Result::ENullptr;
    }

    using namespace Iron::AssetCompiler;

    CCompilerFactory* temp{ new CCompilerFactory() };
    if (!temp) {
        return Iron::Result::ENomemory;
    }

    *factory = temp;

    return Iron::Result::Ok;
}

