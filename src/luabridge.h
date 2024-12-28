#pragma once

#include "lua.hpp"
#include "lurpvariant.h"

#include <assert.h>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <map>

namespace lurp {

class LuaBridge
{
	friend struct TableIt;
public:
	LuaBridge();	
	~LuaBridge();

	void loadLUA(const std::string& path, const std::optional<std::string>& scriptFile);

	lua_State* getLuaState() const { return L; }

	struct StringCount {
		std::string str;
		int count = 0;

		bool operator==(const StringCount& rhs) const {
			return str == rhs.str && count == rhs.count;
		}
	};
	void setGlobal(const std::string& key, const Variant& value);
	void pushGlobal(const std::string& key) const;
	void pushTable(const std::string& key, int index = 0) const;
	void pushNewGlobalTable(const std::string& key);
	void pushNewTable(const std::string& key, int index = 0);
	void pop(int n = 1);

	bool isTable(int index) const { return lua_istable(L, index); }

	void nilGlobal(const std::string& key);

	// Calling a func is a little tricky. The function ref is pushed before the args, which
	// is awkward. The function ref is handy or it isn't. Try to wrap this up in a reasonably
	// complete but simple interface.
	// 
	// Most primitive: this makes the call, but the stack has to already be set up.
	//     funcRef
	//     args[]
	// Returns: error code from Lua. 0 is no error.
	int pCallFunc(int nArgs, int nResults);
	int pCallFuncMultiRet(int nArgs, int& nResults);

	// Takes args in, but leaves results on the stack.
	// Returns the number of results, or -1 for error.
	int callFunc(int funcRef, const std::vector<Variant>& args);
	// Calls the function by name - handy! But has to be global. No changes to stack.
	// Returns number of results, or -1 for error.
	int callGlobalFunc(const std::string& name, const std::vector<Variant>& args);

	bool hasField(const std::string& key) const;
	int getFieldType(const std::string& key) const;

	std::string getStrField(const std::string& key, const std::optional<std::string>& def) const;
	int getIntField(const std::string& key, const std::optional<int>& def) const;
	bool getBoolField(const std::string& key, std::optional<bool>) const;
	std::vector<std::string> getStrArray(const std::string& key) const;
	std::vector<StringCount> getStrCountArray(const std::string& key) const;
	std::vector<int> getIntArray(const std::string& key) const;

	static Variant getField(lua_State* L, const std::string& key, int index, bool raw = false);

	void setStrField(const std::string& key, const std::string& value);
	void setIntField(const std::string& key, int value);
	void setBoolField(const std::string& key, bool value);
	void setStrArray(const std::string& key, const std::vector<std::string>& values);
	void setStrCountArray(const std::string& key, const std::vector<StringCount>& values);
	void setIntArray(const std::string& key, const std::vector<int>& values);

	struct FuncInfo {
		std::string srcName;
		int srcLine = 0;
		int nParams = 0;
	};
	FuncInfo getFuncInfo(int funcRef);
	void registerGlobalFunc(void* handler, lua_CFunction func, const std::string& funcName);

	// Make sure the stack is the same size when this object goes out of scope.
	struct LuaStackCheck {
		LuaStackCheck(lua_State* L, int diff = 0) : _L(L) {
			_nStack = lua_gettop(_L) + diff;
		}
		LuaStackCheck(const LuaStackCheck&) = delete;
		~LuaStackCheck() {
			assert(lua_gettop(_L) == _nStack);
		}

		lua_State* _L;
		int _nStack = 0;
	};

	void doFile(const std::string& filename);
	int getFuncField(const std::string& key) const {
		return getFuncField(L, key);
	}

	void appendLuaPath(const std::string& path);

	// fixme: hack - the currentDir is set and cleared for file loading
	std::filesystem::path currentDir() const {
		return _currentDir;
	}

private:
	lua_State* L = 0;
	std::map<int, FuncInfo> _funcInfoMap;
	std::filesystem::path _currentDir;

	static int getFuncField(lua_State* L, const std::string& key);
	static bool hasField(lua_State* L, const std::string& key);
};

struct TableIt
{
	TableIt(lua_State* L, int index);
	~TableIt();

	bool done() const {
		return _done;
	}
	void next();

	int vType() const {
		return lua_type(_L, -1);
	}

	int kType() const {
		return lua_type(_L, -2);
	}

	Variant key() const;
	Variant value() const;

	bool isTableWithKV(const std::string& key, const std::string& value);

private:
	lua_State* _L;
	bool _done = false;
};
} // namespace lurp
