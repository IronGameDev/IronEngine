#include <Iron.Core/Core.h>
#include <Iron.AssetCompiler/AssetCompiler.h>
#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>
#include <Iron.RHI/RHI.h>

#include <Editor/Assets/AssetRegistry.h>

#pragma comment(lib, "iron.core.lib")
#pragma comment(lib, "iron.engine.lib")

using namespace Iron;
using namespace Iron::RHI;
using namespace Iron::Window;
using namespace Iron::Editor;

class CEditor : public Application {
public:
    ~CEditor() override = default;

    Result::Code PreInitialize() override {
        using namespace Iron::AssetCompiler;
#if _DEBUG
        ICompilerFactory* const assets{ (ICompilerFactory* const)LoadPlugin("D:\\code\\IronEngine\\x64\\Debug\\Iron.AssetCompiler.dll") };
#else
        ICompilerFactory* const assets{ (ICompilerFactory* const)LoadPlugin("D:\\code\\IronEngine\\x64\\Release\\Iron.AssetCompiler.dll") };
#endif
        if (!assets)
            return Result::ELoadlibrary;

        bool d3d12{ true };

        IShaderCompiler* compiler{};
        assets->CreateShaderCompiler({ (u32)(d3d12 ? 6 : 5), (u32)(d3d12 ? 6 : 0), 0 }, &compiler);
        if (!compiler)
            return Result::ECreateResource;

        CompileShaderInfo info{};
        info.Type = RHI::ShaderType::Vertex;
        info.Entry = "FullScreenVS";
        info.Flags = 0;
        info.Defines = nullptr;
        info.NumDefines = 0;

        IShader* shader{};
        compiler->CompileShaderFromFile("D:\\code\\IronEngine\\EngineAssets\\DXCommon\\FullscreenVS.hlsl",
            info, &shader);
        if (shader) shader->SaveToFile(d3d12
            ? "D:\\code\\IronEngine\\EngineAssets\\D3D12\\Bin\\FullscreenVS.bin"
            : "D:\\code\\IronEngine\\EngineAssets\\D3D11\\Bin\\FullscreenVS.bin");
        SafeRelease(shader);

        info.Type = RHI::ShaderType::Pixel;
        info.Entry = "ColorPS";
        compiler->CompileShaderFromFile("D:\\code\\IronEngine\\EngineAssets\\DXCommon\\ColorPS.hlsl",
            info, &shader);
        if (shader) shader->SaveToFile(d3d12
            ? "D:\\code\\IronEngine\\EngineAssets\\D3D12\\Bin\\ColorPS.bin"
            : "D:\\code\\IronEngine\\EngineAssets\\D3D11\\Bin\\ColorPS.bin");
        SafeRelease(shader);

        UnloadPlugin("Iron.AssetCompiler.dll");

        return Result::Code::Ok;
    }

    Result::Code PostInitialize() override {
        Assets::g_AssetRegistry.Load("D:\\code\\IronEngine\\EngineAssets\\Content\\AssetRegistry.json");

        return Result::Code::Ok;
    }

    void Frame() override {
    }

    void Shutdown() override {
    }

private:
};

//TODO: Add compute backend for headless

int main(int argc, char** argv) {
    EngineInitInfo engine_info{};
    engine_info.AppName = "Engine Editor";
    engine_info.AppVersion.Major = 12;
    engine_info.AppVersion.Minor = 42;
    engine_info.AppVersion.Patch = 948;
    engine_info.ArgC = argc;
    engine_info.ArgV = argv;
    engine_info.Headless = false;
    engine_info.Window.Fullscreen = false;
    
    CEditor instance{};

    Result::Code result{};
    result = RunEngine(engine_info, &instance);
    if (!Result::Success(result)) {
        return -1;
    }

    return 0;
}
