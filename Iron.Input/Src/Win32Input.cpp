#include <Iron.Input/Input.h>

#include <Windows.h>

namespace Iron::Input {
}

extern "C" __declspec(dllexport)
Iron::Result::Code
GetFactory() {
    return Iron::Result::Ok;
}
