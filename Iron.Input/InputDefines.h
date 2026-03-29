#pragma once
#include <Iron.Core/Core.h>

namespace Iron::Input {
struct InputType {
    enum Type : u32 {
        Press = 0,
        Release,
        Hold
    };
};

struct Key {
    enum Code : u32 {
        KeyA, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG,
        KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM, KeyN,
        KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU,
        KeyV, KeyW, KeyX, KeyY, KeyZ,
        Key1, Key2, Key3, Key4, Key5,
        Key6, Key7, Key8, Key9, Key0,
        KeyF1, KeyF2, KeyF3, KeyF4, KeyF5,
        KeyF6, KeyF7, KeyF8, KeyF9, KeyF10,
        KeyF11, KeyF12,
        KeyShift, KeyAlt, KeyControl,
        KeyEnter, KeySpace,
        KeyNum1, KeyNum2, KeyNum3, KeyNum4,
        KeyNum5, KeyNum6, KeyNum7, KeyNum8,
        KeyNum9, KeyNum0,
        KeyCaps, KeyHome, KeyPageUp,
        KeyPageDown, KeyDelete, KeyEscape,
        Count
    };
};

struct ModifierKey {
    enum Code : u32 {
        Shift = Key::KeyShift,
        Alt = Key::KeyAlt,
        Control = Key::KeyControl,
    };
};
}
