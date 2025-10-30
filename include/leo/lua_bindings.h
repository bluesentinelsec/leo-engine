// lua_bindings.h - Lua bindings for leo engine/image
#pragma once

#include "leo/export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Registers engine and image bindings into a lua_State on the stack.
// Creates global table `leo` with engine functions and subtable `leo.image`.
LEO_API int leo_LuaOpenBindings(void *L);

#ifdef __cplusplus
}
#endif

