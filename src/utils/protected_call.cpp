#include "protected_call.hpp"

using namespace godot;

namespace luagdextension {

struct ProtectedBody {
	const std::function<void(lua_State *)> *body;
};

static int protected_trampoline(lua_State *L) {
	ProtectedBody *call = (ProtectedBody *) lua_touserdata(L, 1);
	lua_remove(L, 1);
	(*call->body)(L);
	return 0;
}

bool protected_call(lua_State *L, const std::function<void(lua_State *)>& body, String *error) {
	int top = lua_gettop(L);
	ProtectedBody call = { &body };
	// Pushing a light C function and a light userdata never allocates.
	lua_pushcfunction(L, protected_trampoline);
	lua_pushlightuserdata(L, &call);
	int status = lua_pcall(L, 1, 0, 0);
	if (status != LUA_OK) {
		if (error != nullptr) {
			size_t length = 0;
			const char *message = lua_tolstring(L, -1, &length);
			*error = message != nullptr ? String::utf8(message, length) : String("Lua error");
		}
		lua_settop(L, top);
		return false;
	}
	lua_settop(L, top);
	return true;
}

}
