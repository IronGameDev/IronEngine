#include <Iron.Input/Input.h>

#include <Windows.h>

namespace Iron::Input {
class CInputFactory : public IInputFactory {
public:

private:
};
}

extern "C" __declspec(dllexport)
Iron::Result::Code
GetFactory(Iron::Input::IInputFactory** factory) {
    if (!factory) {
        return Iron::Result::ENullptr;
    }

    using namespace Iron::Input;
    CInputFactory* temp{ new CInputFactory() };
    if (!temp) {
        return Iron::Result::ENomemory;
    }

    *factory = temp;

    return Iron::Result::Ok;
}
