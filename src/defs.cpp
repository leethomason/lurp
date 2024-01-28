#include "defs.h"
#include <fmt/core.h>

namespace lurp {

bool Variant::isTruthy() const
{
	switch (type) {
	case LUA_TNIL:
	case LUA_TNONE:
		return false;
	case LUA_TBOOLEAN:
		return boolean;
	case LUA_TNUMBER:
		return num != 0;
	case LUA_TSTRING:
		return !str.empty();
	default:
		return true;
	}
}

/*static*/ Variant Variant::fromLua(lua_State* L, int index)
{
	Variant v;
	switch (lua_type(L, index)) {
	case LUA_TSTRING:
		v.type = LUA_TSTRING;
		v.str = lua_tostring(L, index);
		break;
	case LUA_TNUMBER:
		v.type = LUA_TNUMBER;
		v.num = lua_tonumber(L, index);
		break;
	case LUA_TBOOLEAN:
		v.type = LUA_TBOOLEAN;
		v.boolean = lua_toboolean(L, index);
		break;
	default:
		v.type = LUA_TNIL;
		break;
	}
	return v;
}


void Variant::pushLua(lua_State* L) const
{
	switch (type) {
	case LUA_TSTRING:
		lua_pushstring(L, str.c_str());
		break;
	case LUA_TNUMBER:
		lua_pushnumber(L, num);
		break;
	case LUA_TBOOLEAN:
		lua_pushboolean(L, boolean);
		break;
	default:
		lua_pushnil(L);
		break;
	}
}

void Variant::dump() const
{
	switch (type) {
	case LUA_TSTRING:
		fmt::print("{}", str);
		break;
	case LUA_TNUMBER:
		fmt::print("{}", num);
		break;
	case LUA_TBOOLEAN:
		fmt::print("{}", boolean ? "true" : "false");
		break;
	default:
		fmt::print("nil");
		break;
	}
}

std::string Variant::toLuaString() const
{
	switch (type) {
	case LUA_TSTRING: return fmt::format("\"{}\"", str);
	case LUA_TNUMBER: return fmt::format("{}", num);
	case LUA_TBOOLEAN: return fmt::format("{}", boolean ? "true" : "false");
	case LUA_TNIL: return "nil";
	}
	return "unknown";
}

} // namespace lurp
