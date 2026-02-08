#include <Iron.Core/Core.h>
#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>

#pragma comment(lib, "iron.core.lib")
#pragma comment(lib, "iron.engine.lib")

using namespace iron;
using namespace iron::window;

class editor : public application {
public:
    ~editor() override = default;

    result::code pre_initialize() override {
        return result::code::ok;
    }

    result::code post_initialize() override {
        return result::code::ok;
    }

    void frame() override {
    }

    void shutdown() override {
    }

private:
};

//TODO: Add compute backend for headless
//TODO: Allow user to costumize windows

int main(int argc, char** argv) {
    engine_init_info engine_info{};
    engine_info.app_name = "Engine Editor";
    engine_info.app_version.major = 12;
    engine_info.app_version.minor = 42;
    engine_info.app_version.patch = 948;
    engine_info.argc = argc;
    engine_info.argv = argv;
    engine_info.headless = option::disable;

    editor instance{};

    result::code result{};
    result = run_engine(engine_info, &instance);
    if (!result::success(result)) {
        return -1;
    }

    return 0;
}
