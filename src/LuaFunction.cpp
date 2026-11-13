/**
 * Copyright (C) 2026 Gil Barbosa Reis.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the “Software”), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "LuaFunction.hpp"

#include "LuaError.hpp"
#include "LuaState.hpp"
#include "utils/protected_call.hpp"

#include "LuaDebug.hpp"
#include "utils/VariantArguments.hpp"
#include "utils/convert_godot_lua.hpp"
#include "utils/string_names.hpp"

#include <godot_cpp/core/error_macros.hpp>

namespace luagdextension {

LuaFunction::LuaFunction() : LuaObjectSubclass() {}
LuaFunction::LuaFunction(sol::protected_function&& function) : LuaObjectSubclass(function) {}
LuaFunction::LuaFunction(const sol::protected_function& function) : LuaObjectSubclass(function) {}

void LuaFunction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("invokev", "arg_array"), &LuaFunction::invokev);
	ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "invoke", &LuaFunction::invoke);
	ClassDB::bind_method(D_METHOD("to_callable"), &LuaFunction::to_callable);
	ClassDB::bind_method(D_METHOD("get_debug_info"), &LuaFunction::get_debug_info);
}

Variant LuaFunction::invokev(const Array& args) {
	return invoke_lua(lua_object, args, true);
}

Variant LuaFunction::invoke(const Variant **args, GDExtensionInt arg_count, GDExtensionCallError &error) {
	error.error = GDEXTENSION_CALL_OK;
	return invoke_lua(lua_object, VariantArguments(args, arg_count), true);
}

Variant LuaFunction::invoke_lua(Ref<LuaFunction> f, const VariantArguments& args, bool return_lua_error) {
	return invoke_lua(f->get_function(), args, return_lua_error);
}

Variant LuaFunction::invoke_lua(const sol::protected_function& f, const VariantArguments& args, bool return_lua_error) {
	// Pushing the arguments and converting the results happen outside the inner protected call.
	// Under a memory limit they can fail: run the whole invocation protected, so the error never
	// reaches the panic handler. Without a limit, keep the direct call (hooks see no extra frame).
	LuaState *state = LuaState::find_lua_state(f.lua_state());
	if (state == nullptr || state->get_memory_limit() == 0) {
		sol::protected_function_result result = f.call(args);
		return to_variant(result, return_lua_error);
	}
	Variant value;
	String error;
	bool ok = protected_call(f.lua_state(), [&](lua_State *) {
		sol::protected_function_result result = f.call(args);
		value = to_variant(result, return_lua_error);
	}, &error);
	if (!ok) {
		if (return_lua_error) {
			return memnew(LuaError(LuaError::RUNTIME, error));
		}
		ERR_FAIL_V_MSG(Variant(), error);
	}
	return value;
}

Callable LuaFunction::to_callable() const {
	return Callable((Object *) this, string_names->invoke);
}

Ref<LuaDebug> LuaFunction::get_debug_info() const {
	lua_Debug debug = {};
	LuaDebug::fill_info(lua_object, &debug);
	return memnew(LuaDebug(std::move(debug)));
}

const sol::protected_function& LuaFunction::get_function() const {
	return lua_object;
}

}
