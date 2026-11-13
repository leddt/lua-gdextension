extends RefCounted

const HEADROOM := 256 * 1024

var lua: LuaState


func _setup():
	lua = LuaState.new()
	lua.open_libraries(LuaState.LUA_BASE | LuaState.LUA_STRING | LuaState.LUA_MATH | LuaState.LUA_TABLE)
	lua.collect_garbage()
	lua.set_memory_limit(lua.get_memory_used() + HEADROOM)


func _is_memory_error(result) -> bool:
	return result is LuaError and result.status == LuaError.MEMORY


func test_limit_is_reported() -> bool:
	return lua.get_memory_limit() > HEADROOM


func test_zero_means_no_limit() -> bool:
	lua.set_memory_limit(0)
	var result = lua.do_string("local t = {} for i = 1, 100000 do t[i] = i end return #t")
	return result == 100000 and lua.get_memory_limit() == 0


func test_growing_table_is_refused_at_the_limit() -> bool:
	var result = lua.do_string("local t = {} for i = 1, 1e8 do t[i] = i end")
	return _is_memory_error(result) and lua.get_memory_used() <= lua.get_memory_limit()


func test_single_huge_allocation_is_refused() -> bool:
	var result = lua.do_string("return string.rep('x', 2^28)")
	return _is_memory_error(result) and lua.get_memory_used() <= lua.get_memory_limit()


func test_many_operand_concat_is_refused_before_allocating() -> bool:
	var operands := PackedStringArray()
	for i in 190:
		operands.append("s")
	var source := "local s = string.rep('x', 2000) return " + " .. ".join(operands)
	var result = lua.do_string(source)
	return _is_memory_error(result) and lua.get_memory_used() <= lua.get_memory_limit()


func test_garbage_is_collected_before_refusing() -> bool:
	# Each iteration leaves ~64 KB of garbage: only the emergency collection keeps this under the cap.
	var result = lua.do_string("for i = 1, 200 do local s = string.rep('y', 65536) end return 'ok'")
	return result == "ok"


func test_state_is_usable_after_a_memory_error() -> bool:
	lua.do_string("return string.rep('x', 2^28)")
	return lua.do_string("return 1 + 1") == 2


func test_strings_do_not_coerce_to_numbers() -> bool:
	return lua.do_string("return '1' + 1") is LuaError


func test_create_table_past_the_limit_does_not_crash() -> bool:
	lua.collect_garbage()
	lua.set_memory_limit(lua.get_memory_used() + 1)
	var big := {}
	for i in 1000:
		big["key_%d" % i] = i
	var table = lua.create_table(big)
	lua.set_memory_limit(0)
	return table == null


func test_iterating_a_key_that_breaks_next_does_not_crash() -> bool:
	var t = lua.do_string("return { ['\\xc3'] = 1, b = 2 }")
	var seen := 0
	for key in t:
		seen += 1
	return seen >= 1


func test_state_is_usable_after_a_bad_iteration() -> bool:
	var t = lua.do_string("return { ['\\xc3'] = 1, b = 2 }")
	for key in t:
		pass
	return lua.do_string("return 40 + 2") == 42


func test_state_is_usable_after_create_table_fails() -> bool:
	lua.collect_garbage()
	lua.set_memory_limit(lua.get_memory_used() + 1)
	var big := {}
	for i in 1000:
		big["key_%d" % i] = i
	lua.create_table(big)
	lua.set_memory_limit(0)
	return lua.do_string("return 40 + 2") == 42


func test_invoking_past_the_limit_returns_an_error() -> bool:
	var f = lua.do_string("return function(t) return t end")
	lua.collect_garbage()
	lua.set_memory_limit(lua.get_memory_used() + 1)
	var big := {}
	for i in 1000:
		big["key_%d" % i] = i
	var result = f.invoke(big)
	lua.set_memory_limit(0)
	return result is LuaError and lua.do_string("return 40 + 2") == 42
