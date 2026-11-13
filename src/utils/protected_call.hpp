/**
 * Runs C++ code that touches the Lua API inside `lua_pcall`.
 *
 * A Lua error raised outside any protected call (a refused allocation under a memory limit,
 * "invalid key to 'next'"...) reaches the panic handler, which marks the thread as dead in
 * Lua 5.4: catching the resulting exception is not enough to keep the state usable. Running the
 * code through `lua_pcall` turns those errors into ordinary error results instead.
 */
#ifndef __UTILS_PROTECTED_CALL_HPP__
#define __UTILS_PROTECTED_CALL_HPP__

#include <functional>

#include <godot_cpp/variant/string.hpp>
#include <sol/sol.hpp>

namespace luagdextension {

/// Runs `body` in protected mode. Returns false and fills `error` (when not null) if a Lua error
/// or a C++ exception escaped `body`. The Lua stack is restored to its previous top either way,
/// so `body` must copy out any value it needs.
bool protected_call(lua_State *L, const std::function<void(lua_State *)>& body, godot::String *error = nullptr);

}

#endif
