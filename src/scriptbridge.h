#pragma once

#include "luabridge.h"

#include "zone.h"
#include "scriptdriver.h"
#include "scripttypes.h"
#include "util.h"
#include "geom.h"

#include <vector>
#include <string>
#include <optional>
#include <filesystem>

extern "C" struct lua_State;

namespace lurp {

struct ScriptAssets;
struct ConstScriptAssets;
struct Battler;
class Random;

class ScriptBridge : public LuaBridge
{
public:
	ScriptBridge();
	~ScriptBridge();

	void setIMap(IMapHandler* handler) {
		assert((handler && !_iMapHandler) || (!handler && _iMapHandler));
		_iMapHandler = handler;
	}

	void setICore(ICoreHandler* handler) {
		if (handler) {
			if (_iCoreCount == 0) {
				assert(!_iCoreHandler);
				_iCoreHandler = handler;
			}
			else {
				assert(handler == _iCoreHandler);
			}
			_iCoreCount++;
		}
		else {
			_iCoreCount--;
			if (_iCoreCount == 0) {
				_iCoreHandler = nullptr;
			}
		}
	}

	void setIText(ITextHandler* handler) {
		assert((handler && !_iTextHandler) || (!handler && _iTextHandler));
		_iTextHandler = handler;
	}

	void setIAsset(IAssetHandler* handler) {
		assert((handler && !_iAssetHandler) || (!handler && _iAssetHandler));
		_iAssetHandler = handler;
	}

	ConstScriptAssets readCSA(const std::string& path);
	std::vector<Text> LoadMD(const std::string& filename);


	Point GetPointField(const std::string& key, const std::optional<Point>& def) const;
	Rect GetRectField(const std::string& key, const std::optional<Rect>& def) const;
	Color GetColorField(const std::string& key, const std::optional<Color>& def) const;

	Inventory readInventory() const;
	bool basicTestPassed() const { return _basicTestPassed; }

private:
	bool _basicTestPassed = false;
	IMapHandler* _iMapHandler = nullptr;
	ICoreHandler* _iCoreHandler = nullptr;
	ITextHandler* _iTextHandler = nullptr;
	IAssetHandler* _iAssetHandler = nullptr;
	int _iCoreCount = 0;

	ConstScriptAssets* _currentCSA = nullptr;	// for md callback. hacky.

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

	std::vector<EntityID> readObjects() const;

	static int l_CRandom(lua_State* L);
	static int l_CDeltaItem(lua_State* L);
	static int l_CNumItems(lua_State* L);
	static int l_CCoreGet(lua_State* L);
	static int l_CCoreSet(lua_State* L);
	static int l_CAllTextRead(lua_State* L);
	static int l_CMove(lua_State* L);
	static int l_CEndGame(lua_State* L);
	static int l_CLoadMD(lua_State* L);
};


} // namespace lurp