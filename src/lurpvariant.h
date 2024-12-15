#pragma once

#include <string>

#include "lua.hpp"

namespace lurp {
    struct Variant {
	Variant() = default;
	Variant(const Variant&) = default;
	Variant& operator=(const Variant&) = default;

	Variant(const std::string& s) : type(LUA_TSTRING), str(s) {}
	Variant(const char* s) : type(LUA_TSTRING), str(s) {}
	Variant(double n) : type(LUA_TNUMBER), num(n) {}
	Variant(int n) : type(LUA_TNUMBER), num(n) {}
	Variant(bool b) : type(LUA_TBOOLEAN), boolean(b) {}

	bool operator==(const Variant& rhs) const {
		return type == rhs.type && str == rhs.str && num == rhs.num && boolean == rhs.boolean;
	}
	bool operator!=(const Variant& rhs) const {
		return !(*this == rhs);
	}

	bool isTruthy() const;
	bool asBool() const { return isTruthy(); }

	static Variant fromLua(lua_State* L, int index);
	void pushLua(lua_State* L) const;

	std::string toLuaString() const;
	void dump() const;

	int type = LUA_TNIL;
	std::string str;
	double num = 0;
	bool boolean = false;
};

}