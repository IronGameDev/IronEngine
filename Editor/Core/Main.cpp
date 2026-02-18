#include <Iron.Core/Core.h>
#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>

#pragma comment(lib, "iron.core.lib")
#pragma comment(lib, "iron.engine.lib")

using namespace Iron;
using namespace Iron::Window;

class Editor : public Application {
public:
    ~Editor() override = default;

    Result::Code PreInitialize() override {
        return Result::Code::Ok;
    }

    Result::Code PostInitialize() override {
        
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

    Editor instance{};

    Result::Code result{};
    result = RunEngine(engine_info, &instance);
    if (!Result::Success(result)) {
        return -1;
    }

    return 0;
}
