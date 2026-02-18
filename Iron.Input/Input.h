#pragma once
#include <Iron.Core/Core.h>

MAKE_VERSION(Input, 0, 2, 0)

namespace Iron::Input {
class IInputListener {
public:
    virtual ~IInputListener() = default;

    virtual void OnEvent(Key::Code code, InputType::Type type, Math::V3 value) = 0;
};

class IController {
public:
    virtual ~IController() = default;
};

class IInputFactory {
public:
    virtual ~IInputFactory() = default;

    virtual s64 InputProc(void* hwnd, u32 msg, u64 wparam, u64 lparam) = 0;
};
}//namespace Iron::Input
