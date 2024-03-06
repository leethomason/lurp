#pragma once

#include "zone.h"
#include "scriptdriver.h"
#include "scripttypes.h"
#include "util.h"

#include <vector>
#include <string>
#include <optional>

extern "C" struct lua_State;

namespace lurp {

struct ScriptAssets;
struct ConstScriptAssets;
struct Battler;
class Random;

class ScriptBridge
{
	friend struct TableIt;
	friend class ScriptHelper;
public:
	ScriptBridge();
	~ScriptBridge();

	void setIMap(IMapHandler* handler) {
		assert((handler && !_iMapHandler) || (!handler && _iMapHandler));
		_iMapHandler = handler;
	}

	void setICore(ICoreHandler* handler) {
		assert((handler && !_iCoreHandler) || (!handler && _iCoreHandler));
		_iCoreHandler = handler;
	}

	void setIText(ITextHandler* handler) {
		assert((handler && !_iTextHandler) || (!handler && _iTextHandler));
		_iTextHandler = handler;
	}

	void setIAsset(IAssetHandler* handler) {
		assert((handler && !_iAssetHandler) || (!handler && _iAssetHandler));
		_iAssetHandler = handler;
	}

	void loadLUA(const std::string& path);
	ConstScriptAssets readCSA(const std::string& path);

	lua_State* getLuaState() const { return L; }

	struct StringCount {
		std::string str;
		int count = 0;

		bool operator==(const StringCount& rhs) const {
			return str == rhs.str && count == rhs.count;
		}
	};
	void pushGlobal(const std::string& key);
	void pushTable(const std::string& key, int index = 0);
	void pushNewGlobalTable(const std::string& key);
	void pushNewTable(const std::string& key, int index = 0);
	void pop(int n = 1);

	void nilGlobal(const std::string& key);
	void callGlobalFunc(const std::string& name);	// no return

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

	// Make sure the stack is the same size when this object goes out of scope.
	struct LuaStackCheck {
		LuaStackCheck(lua_State* L, int diff=0) : _L(L) {
			_nStack = lua_gettop(_L) + diff;
		}
		~LuaStackCheck() {
			assert(lua_gettop(_L) == _nStack);
		}

		lua_State* _L;
		int _nStack = 0;
	};

	Inventory readInventory() const;

private:
	lua_State* L = 0;
	std::map<int, FuncInfo> _funcInfoMap;
	IMapHandler* _iMapHandler = nullptr;
	ICoreHandler* _iCoreHandler = nullptr;
	ITextHandler* _iTextHandler = nullptr;
	IAssetHandler* _iAssetHandler = nullptr;

	void registerCallbacks();

	Zone readZone() const;
	Room readRoom() const;
	Edge readEdge() const;
	Container readContainer() const;

	Item readItem() const;
	Power readPower() const;
	Text readText() const;
	Choices readChoices() const;
	Script readScript() const;
	Interaction readInteraction() const;
	Actor readActor() const;
	Combatant readCombatant() const;
	Battle readBattle() const;
	CallScript readCallScript() const;

	EntityID readEntityID(const std::string& key, const std::optional<EntityID>& def) const;
	ScriptType toScriptType(const std::string& type) const;

	static int getFuncField(lua_State* L, const std::string& key);
	static bool hasField(lua_State* L, const std::string& key);

	std::vector<EntityID> readObjects() const;
	void appendLuaPath(const std::string& path);

	void doFile(const std::string& filename);

	static int l_CRandom(lua_State* L);
	static int l_CDeltaItem(lua_State* L);
	static int l_CNumItems(lua_State* L);
	static int l_CCoreGet(lua_State* L);
	static int l_CCoreSet(lua_State* L);
	static int l_CAllTextRead(lua_State* L);
	static int l_CMove(lua_State* L);
	static int l_CEndGame(lua_State* L);
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