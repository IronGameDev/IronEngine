#include <Iron.Engine/Engine.h>
#include <Iron.Windowing/Windowing.h>

namespace iron {
namespace {
engine_module               g_windowing{};
//window::vtable_windowing    g_vtable{};
//window::factory_windowing   g_factory{ &g_vtable };
}//anonymous namespace

void
unload_windowing() {
}
}
