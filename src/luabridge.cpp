#include "luabridge.h"

#include "lua.hpp"
#include "../drivers/platform.h"
#include "debug.h"

#include <plog/Log.h>
#include <fmt/core.h>

namespace lurp {

template<typename T>
void FatalReadError(const std::string& msg, const T& t)
{
	PLOG(plog::error) << fmt::format("Error reading {}: '{}' {}", scriptTypeName(T::type), msg, t.description());
	exit(1);
}

TableIt::TableIt(lua_State* L, int index) : _L(L) {
	assert(lua_type(_L, index) == LUA_TTABLE);
	// we don't have to worry about mutating the stack.
	lua_pushvalue(L, index);
	lua_pushnil(L);
	lua_pushnil(L);	// the next() starts with a pop, THEN replaces the key
	next();
}

TableIt::~TableIt() {
	lua_pop(_L, 1);	// remove the dupe.
}

void TableIt::next() {
	lua_pop(_L, 1);
	_done = lua_next(_L, -2) == 0;
}

Variant TableIt::key() const {
	return Variant::fromLua(_L, -2);
}

Variant TableIt::value() const {
	return Variant::fromLua(_L, -1);
}

bool TableIt::isTableWithKV(const std::string& key, const std::string& value)
{
	bool result = false;
	if (vType() == LUA_TTABLE) {		// -1 table
		lua_pushstring(_L, key.c_str());	// -1 key, -2 table
		int t = lua_gettable(_L, -2);		// -1 value, -2 table 
		if (t == LUA_TSTRING) {
			std::string s = lua_tostring(_L, -1);
			result = s == value;
		}
		lua_pop(_L, 1); // -1 table
	}
	return result;
}

LuaBridge::LuaBridge()
{
	L = luaL_newstate();
	luaL_openlibs(L);
	appendLuaPath("script");

	LuaStackCheck check(L);
}

LuaBridge::~LuaBridge()
{
	lua_close(L);
}

void LuaBridge::registerGlobalFunc(void* handler, lua_CFunction func, const std::string& funcName)
{
	LuaStackCheck check(L);

	lua_pushlightuserdata(L, handler);
	lua_pushcclosure(L, func, 1);
	lua_setglobal(L, funcName.c_str());
}

void LuaBridge::appendLuaPath(const std::string& path)
{
	//fmt::print("appendLuaPath: {}\n", path);

	LuaStackCheck check(L);
	// Thank you stack overflow.
	// A bunch of code to set the pack.path to include the script directory.

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path"); // get field "path" from table at top of stack (-1)
	std::string cur_path = lua_tostring(L, -1); // grab path string from top of stack
	lua_pop(L, 1); // get rid of the string on the stack we just pushed on line 5

	cur_path.append(";"); // do your path magic here
	cur_path.append(path + "/?.lua");

	lua_pushstring(L, cur_path.c_str()); // push the new one
	lua_setfield(L, -2, "path"); // set the field "path" in table at -2 with value at top of stack
	lua_pop(L, 1); // get rid of package table from top of stack

	//fmt::print("Lua path: {}\n", cur_path);
}

void LuaBridge::doFile(const std::string& filename)
{
	LuaStackCheck check(L);

	std::string cwd;
	CheckPath(filename, cwd);
	int error = luaL_loadfile(L, filename.c_str());
	if (error) {
		PLOG(plog::error) << fmt::format("Occurs when calling luaL_loadfile() 0x{:x}", error);
		PLOG(plog::error) << fmt::format("Msg: '{}'", lua_tostring(L, -1));
	}

	std::filesystem::path path = filename;
	_currentDir = path.parent_path();

	error = lua_pcall(L, 0, LUA_MULTRET, 0);
	_currentDir.clear();

	if (error) {
		PLOG(plog::error) << fmt::format("Error Iitializing file '{}'.", filename);
		PLOG(plog::error) << fmt::format("Occurs when calling lua_pcall() 0x{:x}", error);
		PLOG(plog::error) << fmt::format("Msg: '{}'", lua_tostring(L, -1));
	}

}

void LuaBridge::pushNewGlobalTable(const std::string& name)
{
	LuaStackCheck check(L, 1);

	// nil the old global
	lua_pushnil(L);
	lua_setglobal(L, name.c_str());

	// and push a new one
	lua_newtable(L);
	lua_setglobal(L, name.c_str());
	lua_getglobal(L, name.c_str());
}

void LuaBridge::pushNewTable(const std::string& key, int index)
{
	LuaStackCheck check(L, 1);
	assert(!key.empty() || index > 0);

	if (!key.empty()) {
		lua_newtable(L);
		lua_setfield(L, -2, key.c_str());

		lua_pushstring(L, key.c_str());
		int t = lua_gettable(L, -2);
		CHECK(t == LUA_TTABLE);
	}
	else {
		lua_newtable(L);
		lua_seti(L, -2, index);

		lua_pushinteger(L, index);
		int t = lua_gettable(L, -2);
		CHECK(t == LUA_TTABLE);
	}
}

void LuaBridge::setGlobal(const std::string& key, const Variant& value)
{
	LuaStackCheck check(L);
	value.pushLua(L);
	lua_setglobal(L, key.c_str());
}

void LuaBridge::pushGlobal(const std::string& key) const
{
	LuaStackCheck check(L, 1);
	int t = lua_getglobal(L, key.c_str());
	CHECK(t != LUA_TNIL);
}

void LuaBridge::nilGlobal(const std::string& key)
{
	LuaStackCheck check(L);
	lua_pushnil(L);
	lua_setglobal(L, key.c_str());
}

void LuaBridge::callGlobalFunc(const std::string& name)
{
	LuaStackCheck check(L);
	lua_getglobal(L, name.c_str());
	lua_call(L, 0, 0);
}

void LuaBridge::pushTable(const std::string& key, int index) const
{
	assert(!key.empty() || index > 0);
	assert(lua_type(L, -1) == LUA_TTABLE);

	LuaStackCheck check(L, 1);

	if (!key.empty())
		lua_pushstring(L, key.c_str());
	else
		lua_pushinteger(L, index);
	lua_gettable(L, -2);
}

void LuaBridge::pop(int n)
{
	lua_pop(L, n);
}

bool LuaBridge::getBoolField(const std::string& key, std::optional<bool> def) const
{
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TNIL && def) return def.value();
	assert(v.type == LUA_TBOOLEAN);
	return v.boolean;
}

void LuaBridge::setBoolField(const std::string& key, bool value)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));
	lua_pushstring(L, key.c_str());
	lua_pushboolean(L, value);
	lua_settable(L, -3);
}

bool LuaBridge::hasField(const std::string& key) const
{
	return hasField(L, key);
}

/* static */bool LuaBridge::hasField(lua_State* L, const std::string& key)
{
	Variant v = getField(L, key, 0);
	return v.type != LUA_TNIL;
}

int LuaBridge::getFieldType(const std::string& key) const
{
	Variant v = getField(L, key.c_str(), 0);
	return v.type;
}

std::vector<std::string> LuaBridge::getStrArray(const std::string& key) const
{
	std::vector<std::string> r;
	LuaStackCheck check(L);

	lua_pushstring(L, key.c_str());
	lua_gettable(L, -2);

	for (TableIt it(L, -1); !it.done(); it.next()) {
		if (it.kType() == LUA_TNUMBER) {
			assert(it.vType() == LUA_TSTRING);
			std::string result = lua_tostring(L, -1);
			r.push_back(result);
		}
	}
	lua_pop(L, 1);
	return r;
}

void LuaBridge::setStrArray(const std::string& key, const std::vector<std::string>& values)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));

	lua_pushstring(L, key.c_str());					// -1 key
	lua_newtable(L);								// -2 key -1 table
	for (size_t i = 0; i < values.size(); i++) {
		lua_pushnumber(L, int(i) + 1);				// -3 key -2 table -1 index
		lua_pushstring(L, values[i].c_str());		// -4 key -3 table -2 index -1 value
		lua_settable(L, -3);						// -2 key -1 table
	}
	lua_settable(L, -3);							// clear
}

std::vector<int> LuaBridge::getIntArray(const std::string& key) const
{
	std::vector<int> r;
	LuaStackCheck check(L);

	lua_pushstring(L, key.c_str());
	lua_gettable(L, -2);
	for (TableIt it(L, -1); !it.done(); it.next()) {
		if (it.kType() == LUA_TNUMBER) {
			assert(it.vType() == LUA_TNUMBER);
			int result = (int)lua_tonumber(L, -1);
			r.push_back(result);
		}
	}
	lua_pop(L, 1);
	return r;
}

std::vector<LuaBridge::StringCount> LuaBridge::getStrCountArray(const std::string& key) const
{
	std::vector<StringCount> r;
	LuaStackCheck check(L);

	lua_pushstring(L, key.c_str());
	lua_gettable(L, -2);
	for (TableIt it(L, -1); !it.done(); it.next()) {
		if (it.kType() == LUA_TNUMBER) {
			StringCount sc;
			if (it.vType() == LUA_TTABLE) {
				Variant str = getField(L, "", 1);
				Variant num = getField(L, "", 2);
				assert(str.type == LUA_TSTRING);
				assert(num.type == LUA_TNUMBER);
				sc.str = str.str;
				sc.count = (int)num.num;
				r.push_back(sc);
			}
			else {
				sc.str = lua_tostring(L, -1);
				sc.count = 1;
				r.push_back(sc);
			}
		}
	}
	lua_pop(L, 1);
	return r;
}


void LuaBridge::setStrCountArray(const std::string& key, const std::vector<StringCount>& values)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));

	lua_pushstring(L, key.c_str());					// -1 key
	lua_newtable(L);								// -2 key -1 table
	for (int i = 0; i < (int)values.size(); i++) {
		pushNewTable("", i + 1);
		lua_pushstring(L, values[i].str.c_str());
		assert(lua_type(L, -1) == LUA_TSTRING);
		assert(lua_type(L, -2) == LUA_TTABLE);
		lua_seti(L, -2, 1);

		lua_pushnumber(L, values[i].count);
		lua_seti(L, -2, 2);

		pop(1);
	}
	lua_settable(L, -3);							// clear
}

std::string LuaBridge::getStrField(const std::string& key, const std::optional<std::string>& def) const
{
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TNIL && def) return def.value();
	if (v.type != LUA_TSTRING) {
		throw std::runtime_error(fmt::format("Expected string for key '{}'", key));
	}
	return v.str;
}

void LuaBridge::setStrField(const std::string& key, const std::string& value)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));

	lua_pushstring(L, key.c_str());
	lua_pushstring(L, value.c_str());
	lua_settable(L, -3);
}

/*static*/ Variant LuaBridge::getField(lua_State* L, const std::string& key, int index, bool raw)
{
	assert(!key.empty() || index > 0);	// stupid 1 based index check

	LuaStackCheck check(L);
	Variant v;
	int tMinusOne = lua_type(L, -1);
	assert(tMinusOne == LUA_TTABLE); // -1 table
	if (!key.empty())
		lua_pushstring(L, key.c_str());	// -2 table -1 key
	else
		lua_pushnumber(L, index);		// -2 table -1 key/index

	if (raw)
		v.type = lua_rawget(L, -2);		// -2 table -1 value
	else
		v.type = lua_gettable(L, -2);		// -2 table -1 value

	if (v.type == LUA_TSTRING) {
		v.str = lua_tostring(L, -1);
	}
	else if (v.type == LUA_TNUMBER) {
		v.num = lua_tonumber(L, -1);
	}
	else if (v.type == LUA_TBOOLEAN) {
		v.boolean = lua_toboolean(L, -1) ? true : false;
	}
	lua_pop(L, 1);
	return v;
}

/*static*/ int LuaBridge::getFuncField(lua_State* L, const std::string& key)
{
	LuaStackCheck check(L);

	assert(lua_type(L, -1) == LUA_TTABLE);  // -1 table
	lua_pushstring(L, key.c_str());			// -2 table -1 key
	int type = lua_gettable(L, -2);		    // -2 table -1 value
	if (type != LUA_TFUNCTION) {
		lua_pop(L, 1);
		return -1;
	}
	int r = luaL_ref(L, LUA_REGISTRYINDEX);
	return r;
}

int LuaBridge::getIntField(const std::string& key, const std::optional<int>& def) const
{
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TNIL && def) return def.value();
	assert(v.type == LUA_TNUMBER);
	return (int)v.num;
}

void LuaBridge::setIntField(const std::string& key, int value)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	lua_settable(L, -3);
}

void LuaBridge::setIntArray(const std::string& key, const std::vector<int>& values)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));
	lua_pushstring(L, key.c_str());					// -1 key
	lua_newtable(L);								// -2 key -1 table
	for (size_t i = 0; i < values.size(); i++) {
		lua_pushnumber(L, int(i) + 1);				    // -3 key -2 table -1 index
		lua_pushnumber(L, values[i]);				// -4 key -3 table -2 index -1 value
		lua_settable(L, -3);						// -2 key -1 table
	}
	lua_settable(L, -3);							// clear
}

LuaBridge::FuncInfo LuaBridge::getFuncInfo(int funcRef)
{
	if (_funcInfoMap.find(funcRef) == _funcInfoMap.end()) {
		FuncInfo info;
		lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);
		assert(lua_type(L, -1) == LUA_TFUNCTION);

		lua_Debug ar;
		lua_getinfo(L, ">Su", &ar);

		info.srcName = ar.short_src;
		info.srcLine = ar.linedefined;
		info.nParams = ar.nparams;

		_funcInfoMap[funcRef] = info;
	}
	return _funcInfoMap.at(funcRef);
}

void LuaBridge::loadLUA(const std::string& inputFilePath)
{
	PLOG(plog::info) << fmt::format("Loading: '{}'", inputFilePath);

	doFile("script/_map.lua");
	assert(!inputFilePath.empty());

	std::filesystem::path path = inputFilePath;

	// game/start.lua
	// parent_path:    `./game`
	// need to append: 'game'
	appendLuaPath(path.parent_path().string());

	doFile(inputFilePath);
}

} // namespace lurp