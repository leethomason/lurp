#include "lua.hpp"
#include "scriptbridge.h"

#include "zone.h"
#include "scriptasset.h"
#include "util.h"
#include "../drivers/platform.h"
#include "markdown.h"

#include <plog/Log.h>

#include <fmt/core.h>
#include <vector>
#include <assert.h>
#include <filesystem>
#include <array>

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

#if 0
static void Dump(lua_State* L, int index)
{
	int t = lua_type(L, index);
	switch (t) {
	case LUA_TSTRING:
		fmt::print("string: {}", lua_tostring(L, index));
		break;
	case LUA_TBOOLEAN:
		fmt::print("boolean: {}", lua_toboolean(L, index));
		break;
	case LUA_TNUMBER:
		fmt::print("number: {}", lua_tonumber(L, index));
		break;
	case LUA_TTABLE:
		fmt::print("table: {}", lua_topointer(L, index));
		break;
	default:
		fmt::print("unknown: {}", lua_topointer(L, index));
		break;
	}
}

static void DumpStack(lua_State* L, int n) {
	for (int i = 1; i <= n; i++) {
		fmt::print("  {}: ", -i);
		Dump(L, -i);
		fmt::print("\n");
	}
}
#endif

ScriptBridge::ScriptBridge()
{
	L = luaL_newstate();
	luaL_openlibs(L);
	LuaStackCheck check(L);

	lua_pushstring(L, "script/");
	lua_setglobal(L, "DIR");
	appendLuaPath("script");
	registerCallbacks();

	_basicTestPassed = true;
	doFile("script/_test.lua");
	{
		LuaStackCheck check2(L);
		lua_getglobal(L, "_Test");									// push "tests" table (stack=1)
		if (!lua_istable(L, -1)) {
			PLOG(plog::error) << "Could not find the global 'Test' table";
			_basicTestPassed = false;
		}

		std::string t0 = getStrField("name", {});						// Get field (stack=2)
		CHECK(t0 == "TestName");
		if (t0 != "TestName") _basicTestPassed = false;

		lua_pop(L, 1);												// pop 2 (stack=0)
	}
	{
		LuaStackCheck check2(L);
		lua_getglobal(L, "_Test");									// push "tests" table (stack=1)
		int count = 0;
		for (TableIt it(L, -1); !it.done(); it.next()) {
			++count;
		}
		CHECK(count == 6);
		if (count != 6) _basicTestPassed = false;

		count = 0;
		for (TableIt it(L, -1); !it.done(); it.next()) {
			if (it.kType() == LUA_TNUMBER)
				++count;
		}
		CHECK(count == 4);

		CHECK(getField(L, "iVal", 0).type == LUA_TNUMBER);
		CHECK(getField(L, "", 4).type == LUA_TSTRING);
		if (count != 4) _basicTestPassed = false;

		lua_pop(L, 1);												// pop 2 (stack=0)
	}
	//fmt::print("Script engine init.\n");
}

ScriptBridge::~ScriptBridge()
{
	lua_close(L);
}

void ScriptBridge::registerCallbacks()
{
	struct Func {
		const char* name;
		lua_CFunction func;
	};

	LuaStackCheck check(L);

	// Register c callback funcs
	static const int NUM_FUNCS = 9;
	static const Func funcs[NUM_FUNCS] = {
		{ "CRandom", &l_CRandom },
		{ "CDeltaItem", &l_CDeltaItem},
		{ "CNumItems", &l_CNumItems},
		{ "CCoreGet", &l_CCoreGet},
		{ "CCoreSet", &l_CCoreSet},
		{ "CAllTextRead", &l_CAllTextRead},
		{ "CMove", &l_CMove},
		{ "CEndGame", &l_CEndGame},
		{ "CLoadMD", &l_CLoadMD}
	};
	for (int i = 0; i < NUM_FUNCS; i++) {
		lua_pushlightuserdata(L, this);
		lua_pushcclosure(L, funcs[i].func, 1);
		lua_setglobal(L, funcs[i].name);
	}
}

void ScriptBridge::appendLuaPath(const std::string& path)
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

void ScriptBridge::doFile(const std::string& filename)
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

void ScriptBridge::pushNewGlobalTable(const std::string& name)
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

void ScriptBridge::pushNewTable(const std::string& key, int index)
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

void ScriptBridge::setGlobal(const std::string& key, const Variant& value)
{
	LuaStackCheck check(L);
	value.pushLua(L);
	lua_setglobal(L, key.c_str());
}

void ScriptBridge::pushGlobal(const std::string& key) const
{
	LuaStackCheck check(L, 1);
	int t = lua_getglobal(L, key.c_str());
	CHECK(t != LUA_TNIL);
}

void ScriptBridge::nilGlobal(const std::string& key)
{
	LuaStackCheck check(L);
	lua_pushnil(L);
	lua_setglobal(L, key.c_str());
}

void ScriptBridge::callGlobalFunc(const std::string& name)
{
	LuaStackCheck check(L);
	lua_getglobal(L, name.c_str());
	lua_call(L, 0, 0);
}

void ScriptBridge::pushTable(const std::string& key, int index) const
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

void ScriptBridge::pop(int n)
{
	lua_pop(L, n);
}

bool ScriptBridge::getBoolField(const std::string& key, std::optional<bool> def) const
{
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TNIL && def) return def.value();
	assert(v.type == LUA_TBOOLEAN);
	return v.boolean;
}

void ScriptBridge::setBoolField(const std::string& key, bool value)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));
	lua_pushstring(L, key.c_str());
	lua_pushboolean(L, value);
	lua_settable(L, -3);
}

bool ScriptBridge::hasField(const std::string& key) const
{
	return hasField(L, key);
}

/* static */bool ScriptBridge::hasField(lua_State* L, const std::string& key)
{
	Variant v = getField(L, key, 0);
	return v.type != LUA_TNIL;
}

int ScriptBridge::getFieldType(const std::string& key) const
{
	Variant v = getField(L, key.c_str(), 0);
	return v.type;
}

std::vector<std::string> ScriptBridge::getStrArray(const std::string& key) const
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

void ScriptBridge::setStrArray(const std::string& key, const std::vector<std::string>& values)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));

	lua_pushstring(L, key.c_str());					// -1 key
	lua_newtable(L);								// -2 key -1 table
	for (size_t i = 0; i < values.size(); i++) {
		lua_pushnumber(L, int(i) + 1);				    // -3 key -2 table -1 index
		lua_pushstring(L, values[i].c_str());		// -4 key -3 table -2 index -1 value
		lua_settable(L, -3);						// -2 key -1 table
	}
	lua_settable(L, -3);							// clear
}

std::vector<int> ScriptBridge::getIntArray(const std::string& key) const
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

Point ScriptBridge::GetPointField(const std::string& key, const std::optional<Point>& def) const
{
	Point p;
	LuaStackCheck check(L);

	std::vector<int> vec = getIntArray(key);
	if (vec.size() == 2) {
		p.x = vec[0];
		p.y = vec[1];
	}
	else if (def) {
		p = def.value();
	}
	return p;
}

Rect ScriptBridge::GetRectField(const std::string& key, const std::optional<Rect>& def) const
{
	Rect r;
	LuaStackCheck check(L);

	std::vector<int> vec = getIntArray(key);
	if (vec.size() == 4) {
		r.x = vec[0];
		r.y = vec[1];
		r.w = vec[2];
		r.h = vec[3];
	}
	else if (def) {
		r = def.value();
	}
	return r;
}

Color ScriptBridge::GetColorField(const std::string& key, const std::optional<Color>& def) const
{
	Color c;
	LuaStackCheck check(L);

	std::vector<int> vec = getIntArray(key);
	if (vec.size() == 4) {
		c.r = (uint8_t) vec[0];
		c.g = (uint8_t) vec[1];
		c.b = (uint8_t) vec[2];
		c.a = (uint8_t) vec[3];
	}
	else if (def) {
		c = def.value();
	}
	return c;
}

void ScriptBridge::setIntArray(const std::string& key, const std::vector<int>& values)
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

std::vector<ScriptBridge::StringCount> ScriptBridge::getStrCountArray(const std::string& key) const
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


void ScriptBridge::setStrCountArray(const std::string& key, const std::vector<StringCount>& values)
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

std::string ScriptBridge::getStrField(const std::string& key, const std::optional<std::string>& def) const
{
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TNIL && def) return def.value();
	if (v.type != LUA_TSTRING) {
		throw std::runtime_error(fmt::format("Expected string for key '{}'", key));
	}
	return v.str;
}

void ScriptBridge::setStrField(const std::string& key, const std::string& value)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));

	lua_pushstring(L, key.c_str());
	lua_pushstring(L, value.c_str());
	lua_settable(L, -3);
}

/*static*/ Variant ScriptBridge::getField(lua_State* L, const std::string& key, int index, bool raw)
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

/*static*/ int ScriptBridge::getFuncField(lua_State* L, const std::string& key)
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

int ScriptBridge::getIntField(const std::string& key, const std::optional<int>& def) const
{
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TNIL && def) return def.value();
	assert(v.type == LUA_TNUMBER);
	return (int)v.num;
}

void ScriptBridge::setIntField(const std::string& key, int value)
{
	LuaStackCheck check(L);
	assert(lua_istable(L, -1));
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	lua_settable(L, -3);
}

std::vector<EntityID> ScriptBridge::readObjects() const
{
	std::vector<EntityID> obj;
	LuaStackCheck check(L);

	for (TableIt it(L, -1); !it.done(); it.next()) {
		if (lua_istable(L, -1)) {
			Variant v = getField(L, "entityID", 0);
			assert(v.type == LUA_TSTRING);
			obj.push_back(v.str);
		}
	}
	return obj;
}

Edge ScriptBridge::readEdge() const
{
	Edge re;
	LuaStackCheck check(L);

	try {
		re.entityID = readEntityID("entityID", {});
		std::string dir = toLower(getStrField("dir", { "" }));

		if (!dir.empty()) {
			if (dir == "north" || dir == "n") re.dir = Edge::Dir::kNorth;
			else if (dir == "northeast" || dir == "ne") re.dir = Edge::Dir::kNortheast;
			else if (dir == "east" || dir == "e") re.dir = Edge::Dir::kEast;
			else if (dir == "southeast" || dir == "se") re.dir = Edge::Dir::kSoutheast;
			else if (dir == "south" || dir == "s") re.dir = Edge::Dir::kSouth;
			else if (dir == "southwest" || dir == "sw") re.dir = Edge::Dir::kSouthwest;
			else if (dir == "west" || dir == "w") re.dir = Edge::Dir::kWest;
			else if (dir == "northwest" || dir == "nw") re.dir = Edge::Dir::kNorthwest;
			else assert(false);
		}

		re.name = getStrField("name", { "" });
		re.room1 = getStrField("room1", {});
		re.room2 = getStrField("room2", {});
		re.locked = getBoolField("locked", { false });
		re.key = getStrField("key", { "" });
	}
	catch(std::exception& e) {
		FatalReadError(e.what(), re);
	}
	return re;
}

Container ScriptBridge::readContainer() const
{
	Container c;
	LuaStackCheck check(L);

	try {
		c.entityID = readEntityID("entityID", {});
		c.name = getStrField("name", {});
		c.locked = getBoolField("locked", { false });
		c.key = getStrField("key", { "" });
		c.eval = getFuncField(L, "eval");
		c.inventory = readInventory();
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), c);
	}
	return c;
}

Inventory ScriptBridge::readInventory() const
{
	Inventory inv;
	try {
		if (hasField("items")) {
			std::vector<StringCount> arr = getStrCountArray("items");
			for (const auto& p : arr) {
				inv.addInitItem(p.str, p.count);
			}
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), inv);
	}
	return inv;
}

Room ScriptBridge::readRoom() const
{
	Room r;
	LuaStackCheck check(L);

	try {
		r.entityID = readEntityID("entityID", {});
		r.name = getStrField("name", {});
		r.desc = getStrField("desc", { "" });
		r.objects = readObjects();
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), r);
	}
	return r;
}

Zone ScriptBridge::readZone() const
{
	Zone z;
	LuaStackCheck check(L);
	try {
		z.entityID = readEntityID("entityID", {});
		z.objects = readObjects();
		z.name = getStrField("name", { "" });
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), z);
	}
	return z;
}

Item ScriptBridge::readItem() const
{
	Item item;
	LuaStackCheck check(L);
	try {
		item.entityID = readEntityID("entityID", {});
		item.name = getStrField("name", {});
		item.desc = getStrField("desc", { "" });
		item.range = getIntField("range", { 0 });
		item.armor = getIntField("armor", { 0 });
		item.damage = Die::parse(getStrField("damage", { "" }));
		item.ap = getIntField("ap", { 0 });
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), item);
	}
	return item;
}

Power ScriptBridge::readPower() const
{
	Power power;
	LuaStackCheck check(L);
	try {
		power.entityID = readEntityID("entityID", {});
		power.name = getStrField("name", {});
		power.effect = getStrField("effect", {});
		power.cost = getIntField("cost", { 1 });
		power.range = getIntField("range", { 1 });
		power.strength = getIntField("strength", { 1 });
		power.region = getBoolField("region", { false });
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), power);
	}
	return power;
}

Text ScriptBridge::readText() const
{
	Text text;
	LuaStackCheck check(L);
	try {
		/*
			Text {
				entityID = "T1",
				eval = function() return true end,
				test = "{player.alive}",
				code = function() player:set("sun", true) end,
				s = "narrator",

				{ eval = function() return true end, s = "narrator", "It is a beautiful day", "The birds are singing" },
				{ test = "{player.energetic}",	s = "player", "I should go outside" },
				{ code = function() zone:set("outside", true) end, s = "narrator", "You go outside" }

				"This is also text",
				"That the narrator says.",
			}
		*/

		text.entityID = readEntityID("entityID", {});
		text.eval = getFuncField(L, "eval");
		text.test = getStrField("test", { "" });
		text.code = getFuncField(L, "code");
		std::string speaker = getStrField("s", { "" });

		if (hasField("md")) {
			std::string md = getStrField("md", {});
			std::vector<Text::Line> lines = Text::parseMarkdown(md);

			for (const Text::Line& line : lines) {
				text.lines.push_back(line);
			}
		}

		for (TableIt it(L, -1); !it.done(); it.next()) {
			if (it.kType() != LUA_TNUMBER)
				continue;
			if (it.vType() == LUA_TTABLE) {
				Text::Line line;
				if (hasField("s")) {
					line.speaker = getStrField("s", {});
				}
				if (hasField("test")) {
					line.test = getStrField("test", {});
				}
				if (hasField("eval")) {
					line.eval = getFuncField(L, "eval");
				}
				if (hasField("code")) {
					line.code = getFuncField(L, "code");
				}

				// This inner loop goes through each of the strings,
				// which are the actual text lines, assigned to the same
				// speaker with the same eval() and code().
				for (TableIt inner(L, -1); !inner.done(); inner.next()) {
					if (inner.kType() != LUA_TNUMBER) continue;
					Variant v = inner.value();
					if (v.type != LUA_TSTRING) continue;
					line.text = v.str;
					text.lines.push_back(line);
				}
			}
			else if (it.vType() == LUA_TSTRING) {

				Variant v = it.value();
				Text::Line line;
				line.speaker = speaker;
				line.text = v.str;
				text.lines.push_back(line);
			}
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), text);
	}
	return text;
}

ScriptType ScriptBridge::toScriptType(const std::string& type) const
{
	if (type == "Script") return ScriptType::kScript;
	if (type == "Text") return ScriptType::kText;
	if (type == "Choices") return ScriptType::kChoices;
	if (type == "Battle") return ScriptType::kBattle;
	if (type == "CallScript") return ScriptType::kCallScript;
	assert(false);
	return ScriptType::kScript;
}

Script ScriptBridge::readScript() const
{
	Script s;
	LuaStackCheck check(L);
	try {
		s.entityID = readEntityID("entityID", {});
		s.code = getFuncField(L, "code");
		s.npc = getStrField("npc", {""});

		for (TableIt it(L, -1); !it.done(); it.next()) {
			if (it.kType() == LUA_TNUMBER) {
				EntityID id = readEntityID("entityID", {});
				ScriptType type = toScriptType(getStrField("type", {}));
				s.events.push_back({ id, type });
			}
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), s);
	}
	return s;
}

EntityID ScriptBridge::readEntityID(const std::string& key, const std::optional<EntityID>& def) const
{
	std::string id;
	int tMinusOne = lua_type(L, -1);
	if (tMinusOne != LUA_TTABLE) {
		assert(tMinusOne == LUA_TTABLE); // -1 table
		throw std::runtime_error("Error parsing entity");
	}
	
	Variant v = getField(L, key, 0);
	if (v.type == LUA_TSTRING) {
		id = v.str;
	}
	else if (v.type == LUA_TTABLE) {
		lua_pushstring(L, key.c_str());
		int t = lua_gettable(L, -2);
		CHECK(t == LUA_TTABLE);
		id = getStrField("entityID", {});
		lua_pop(L, 1);
	}
	else {
		if (def) 
			id = def.value();
		else
			throw std::runtime_error(fmt::format("No entity found while reading EntityID for key '{}'", key));
	}
	return id;
}

Choices ScriptBridge::readChoices() const
{
	Choices c;
	LuaStackCheck check(L);
	try {
		c.entityID = getStrField("entityID", {});

		std::string a = getStrField("action", { "done" });
		if (a == "done") c.action = Choices::Action::kDone;
		else if (a == "rewind") c.action = Choices::Action::kRewind;
		else if (a == "repeat") c.action = Choices::Action::kRepeat;
		else if (a == "pop") c.action = Choices::Action::kPop;

		for (TableIt it(L, -1); !it.done(); it.next()) {
			if (it.kType() == LUA_TNUMBER) {
				Choices::Choice choice;
				choice.text = getStrField("text", {});
				choice.next = readEntityID("next", { "done" });
				choice.eval = getFuncField(L, "eval");
				choice.code = getFuncField(L, "code");
				c.choices.push_back(choice);
			}
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), c);
	}
	return c;
}

Interaction ScriptBridge::readInteraction() const
{
	Interaction i;
	LuaStackCheck check(L);
	try {
		i.entityID = getStrField("entityID", {});
		i.name = getStrField("name", { "" });
		i.next = readEntityID("next", {});
		i.npc = readEntityID("npc", { "" });
		i.required = getBoolField("required", { false });
		i.eval = getFuncField(L, "eval");
		i.code = getFuncField(L, "code");
	}
	catch(std::exception& e) {
		FatalReadError(e.what(), i);
	}
	return i;
}

Actor ScriptBridge::readActor() const
{
	Actor actor;
	LuaStackCheck check(L);
	try {
		actor.entityID = getStrField("entityID", {});
		actor.name = getStrField("name", {});
		actor.wild = getBoolField("wild", { false });
		actor.fighting = getIntField("fighting", { 0 });
		actor.shooting = getIntField("shooting", { 0 });
		actor.arcane = getIntField("arcane", { 0 });

		if (hasField("items")) {
			actor.inventory = readInventory();
		}
		if (hasField("powers")) {
			lua_pushstring(L, "powers");
			lua_gettable(L, -2);

			for (TableIt it(L, -1); !it.done(); it.next()) {
				EntityID id = it.value().str;
				actor.powers.push_back(id);
			}
			lua_pop(L, 1);
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), actor);
	}
	return actor;
}

Combatant ScriptBridge::readCombatant() const
{
	Combatant c;
	LuaStackCheck check(L);
	try {
		c.entityID = getStrField("entityID", {});
		c.name = getStrField("name", {});
		c.count = getIntField("count", { 1 });
		c.wild = getBoolField("wild", { false });
		c.fighting = getIntField("fighting", { 0 });
		c.shooting = getIntField("shooting", { 0 });
		c.arcane = getIntField("arcane", { 0 });
		c.bias = getIntField("bias", { 0 });

		c.inventory = readInventory();

		if (hasField("powers")) {
			lua_pushstring(L, "powers");
			lua_gettable(L, -2);

			for (TableIt it(L, -1); !it.done(); it.next()) {
				EntityID id = it.value().str;
				c.powers.push_back(id);
			}
			lua_pop(L, 1);
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), c);
	}
	return c;
}

Battle ScriptBridge::readBattle() const
{
	Battle b;
	LuaStackCheck check(L);

	try {
		b.entityID = readEntityID("entityID", {});
		b.name = getStrField("name", {"battle"});

		if (hasField("regions")) {
			lua_pushstring(L, "regions");
			lua_gettable(L, -2);

			for (TableIt it(L, -1); !it.done(); it.next()) {
				if (it.kType() == LUA_TNUMBER) {
					lurp::swbattle::Region r;
					r.name = getField(L, "", 1).str;
					r.yards = (int)getField(L, "", 2).num;
					std::string c = getField(L, "", 3).str;
					if (c == "light") r.cover = lurp::swbattle::Cover::kLightCover;
					else if (c == "medium") r.cover = lurp::swbattle::Cover::kMediumCover;
					else if (c == "heavy") r.cover = lurp::swbattle::Cover::kHeavyCover;

					b.regions.push_back(r);
				}
			}
			lua_pop(L, 1);
		}

		if (hasField("combatants")) {
			std::vector<StringCount> sc = getStrCountArray("combatants");
			for (const auto& s : sc) {
				for (int i = 0; i < s.count; ++i) {
					b.combatants.push_back(s.str);
				}
			}
		}

		for (TableIt it(L, -1); !it.done(); it.next()) {
			if (it.kType() == LUA_TNUMBER) {
				EntityID id = readEntityID("entityID", {});
				b.combatants.push_back(id);
			}
		}
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), b);
	}
	return b;
}

CallScript ScriptBridge::readCallScript() const
{
	CallScript cs;
	LuaStackCheck check(L);
	try {
		cs.entityID = readEntityID("entityID", {});
		cs.scriptID = readEntityID("scriptID", {});
		cs.npc = readEntityID("npc", { "" });
		cs.code = getFuncField(L, "code");
		cs.eval = getFuncField(L, "eval");
	}
	catch (std::exception& e) {
		FatalReadError(e.what(), cs);
	}
	return cs;
}

ScriptBridge::FuncInfo ScriptBridge::getFuncInfo(int funcRef)
{
	LuaStackCheck check(L);

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


// READ_ASSET(AllChoices, Choices, choices, readChoices);
#define READ_ASSET(glob, type, member, func) \
{  \
	lua_getglobal(L, glob); \
	for (TableIt it(L, -1); !it.done(); it.next()) { \
		if (it.isTableWithKV("type", type)) { \
			csa.member.push_back(func()); \
		} \
	} \
	lua_pop(L, 1); \
}

void ScriptBridge::loadLUA(const std::string& inputFilePath)
{
	PLOG(plog::info) << fmt::format("Loading: '{}'", inputFilePath);

	LuaStackCheck check(L);
	doFile("script/_map.lua");
	assert(!inputFilePath.empty());

	std::filesystem::path path = inputFilePath;

	// game/start.lua
	// parent_path:    `./game`
	// need to append: 'game'
	appendLuaPath(path.parent_path().string());

	doFile(inputFilePath);
}


ConstScriptAssets ScriptBridge::readCSA(const std::string& inputFilePath)
{
	assert(!inputFilePath.empty());
	ConstScriptAssets csa;
	LuaStackCheck check(L);

	// required
	doFile("script/_map.lua");

	std::filesystem::path path = inputFilePath;

	// game/start.lua
	// parent_path:    `./game`
	// need to append: 'game'
	appendLuaPath(path.parent_path().string());
	_currentCSA = &csa;
	doFile(inputFilePath);
	_currentCSA = nullptr;

	READ_ASSET("Scripts", "Script", scripts, readScript);
	READ_ASSET("Texts", "Text", texts, readText);
	READ_ASSET("AllChoices", "Choices", choices, readChoices);
	READ_ASSET("Items", "Item", items, readItem);
	READ_ASSET("Powers", "Power", powers, readPower);
	READ_ASSET("Interactions", "Interaction", interactions, readInteraction);

	READ_ASSET("Containers", "Container", containers, readContainer);
	READ_ASSET("Edges", "Edge", edges, readEdge);
	READ_ASSET("Rooms", "Room", rooms, readRoom);
	READ_ASSET("Zones", "Zone", zones, readZone);

	READ_ASSET("Actors", "Actor", actors, readActor);
	READ_ASSET("Combatants", "Combatant", combatants, readCombatant);
	READ_ASSET("Battles", "Battle", battles, readBattle);
	READ_ASSET("CallScripts", "CallScript", callScripts, readCallScript);

	// Now all the inventories need to be converted.
	for (auto& i : csa.actors) i.inventory.convert(csa);
	for (auto& i : csa.combatants) i.inventory.convert(csa);
	for (auto& i : csa.containers) i.inventory.convert(csa);

	int kbytes = lua_gc(L, LUA_GCCOUNT);
	PLOG(plog::info) << fmt::format("LUA memory usage: {} KB", kbytes);

	return csa;
}

std::vector<Text> ScriptBridge::LoadMD(const std::string& filename)
{
	LuaStackCheck check(L);

	std::string text;
	std::filesystem::path path = _currentDir / filename;
	try {
		std::ifstream stream(path);
		std::stringstream buffer;
		buffer << stream.rdbuf();
		text = buffer.str();
	}
	catch (std::exception& e) {
		PLOG(plog::error) << fmt::format("Error loading MD file '{}': {}", path.string(), e.what());
	}

	std::vector<Text> tVec;
	if (!text.empty()) {
		tVec = Text::parseMarkdownFile(text);
	}
	return tVec;
}

/*static*/ int ScriptBridge::l_CRandom(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));
	uint32_t r = 0;
	assert(bridge->_iMapHandler);
	if (bridge->_iMapHandler) {
		r = bridge->_iMapHandler->getRandom();
	}
	lua_pushinteger(L, r);
	return 1;
}

/*static*/ int ScriptBridge::l_CDeltaItem(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));
	assert(bridge->_iMapHandler);

	std::string containerID = lua_tostring(L, 1);
	std::string itemID = lua_tostring(L, 2);
	int n = (int)lua_tointeger(L, 3);

	if (bridge->_iMapHandler) {
		bridge->_iMapHandler->deltaItem(containerID, itemID, n);
	}
	return 0;
}

/*static*/ int ScriptBridge::l_CNumItems(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));
	assert(bridge->_iMapHandler);
	std::string containerID = lua_tostring(L, 1);
	std::string itemID = lua_tostring(L, 2);
	int n = 0;
	if (bridge->_iMapHandler) {
		n = bridge->_iMapHandler->numItems(containerID, itemID);
	}
	lua_pushinteger(L, n);
	return 1;
}

/*static*/ int ScriptBridge::l_CCoreGet(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));
	assert(bridge->_iCoreHandler);
	std::string entityID = lua_tostring(L, 1);
	std::string scope = lua_tostring(L, 2);

	bool handled = false;
	Variant v;

	if (!handled && bridge->_iCoreHandler) {
		std::pair<bool, Variant> p = bridge->_iCoreHandler->coreGet(entityID, scope);
		handled = p.first;
		v = p.second;
	}
	if (!handled && bridge->_iAssetHandler) {
		std::pair<bool, Variant> p = bridge->_iAssetHandler->assetGet(entityID, scope);
		handled = p.first;
		v = p.second;
	}

	lua_pushboolean(L, handled);
	v.pushLua(L);
	return 2;
}

/*static*/ int ScriptBridge::l_CCoreSet(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));
	assert(bridge->_iCoreHandler);
	std::string entityID = lua_tostring(L, 1);
	std::string scope = lua_tostring(L, 2);
	Variant v = Variant::fromLua(L, 3);
	bool mutableUser = lua_toboolean(L, 4);

	bool handled = false;

	if (!handled && bridge->_iCoreHandler) {
		bridge->_iCoreHandler->coreSet(entityID, scope, v, mutableUser);
	}
	return 0;
}

/*static*/ int ScriptBridge::l_CAllTextRead(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));
	bool result = true;
	std::string entityID = lua_tostring(L, 1);

	if (bridge->_iTextHandler) {
		result = bridge->_iTextHandler->allTextRead(entityID);
	}
	lua_pushboolean(L, result);
	return 1;
}

/*static*/ int ScriptBridge::l_CMove(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));

	std::string dstID = lua_tostring(L, 1);
	bool tele = lua_toboolean(L, 2);
	if (bridge->_iMapHandler)
		bridge->_iMapHandler->movePlayer(dstID, tele);
	return 0;
}

/*static*/ int ScriptBridge::l_CEndGame(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));

	std::string reason = lua_tostring(L, 1);
	int bias = (int) lua_tointeger(L, 2);
	if (bridge->_iMapHandler)
		bridge->_iMapHandler->endGame(reason, bias);
	return 0;
}

/*static*/ int ScriptBridge::l_CLoadMD(lua_State* L)
{
	ScriptBridge* bridge = (ScriptBridge*)lua_touserdata(L, lua_upvalueindex(1));

	std::string fname = lua_tostring(L, 1);

	std::vector<Text> tVec = bridge->LoadMD(fname);
	assert(bridge->_currentCSA);
	bridge->_currentCSA->texts.insert(bridge->_currentCSA->texts.end(), tVec.begin(), tVec.end());
	return 0;
}


} // namespace lurp
