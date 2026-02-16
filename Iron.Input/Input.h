#pragma once
#include <Iron.Core/Core.h>

MAKE_VERSION(input, 0, 2, 0)

namespace iron::input {
typedef class input_state_t* input_state;

using input_callback = void(*)(input_type, key::code, modifier_key, void*);

struct vtable_input {
    version api_version;

    result::code(*initialize)();
    void(*shutdown)();

    void(*input_proc)(void*, u32, u64, u64);
    
    void(*register_callback)(input_callback callback, void* user_data);
    void(*unregister_callback)(input_callback callback);
};
} // namespace iron::input
