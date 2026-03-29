#pragma once
#include <Iron.Input/InputDefines.h>

MAKE_VERSION(Input, 0, 2, 0)

namespace Iron::Input {
class IController {
public:
    virtual ~IController() = default;
};

class IInputFactory {
public:
    virtual ~IInputFactory() = default;

    //virtual Result::Code SearchNewController() = 0;
};
}//namespace Iron::Input
