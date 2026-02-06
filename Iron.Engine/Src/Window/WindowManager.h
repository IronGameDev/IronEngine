#pragma once
#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>

namespace iron::_window {
result::code initialize(const char* search_dir);
void shutdown();

result::code add_window(const window_init_info& init_info, window* handle);
}
