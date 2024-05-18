#include <assert.h>
#include <algorithm>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include "zonedriver.h"
#include "items.h"
#include "scriptasset.h"
#include "scripthelper.h"
#include "scriptbridge.h"

namespace lurp {

ZoneDriver::ZoneDriver(ScriptAssets& assets, ScriptBridge& bridge) 
	: _assets(assets), _bridge(bridge), mapData(MapData::kSeed)
{
	_bridge.setIMap(this);
	_bridge.setIAsset(&assets);
	_bridge.setICore(&mapData.coreData);
}

ZoneDriver::ZoneDriver(ScriptAssets& assets, ScriptBridge& bridge, const EntityID& zone) 
	: _assets(assets), _bridge(bridge), mapData(MapData::kSeed)
{
	_bridge.setIMap(this);
	_bridge.setIAsset(&assets);
	_bridge.setICore(&mapData.coreData);
	setZone(zone, EntityID());
	checkScriptDriver();
}

ZoneDriver::~ZoneDriver()
{
	delete _scriptDriver; _scriptDriver = nullptr;
	_bridge.setICore(nullptr);
	_bridge.setIMap(nullptr);
	_bridge.setIAsset(nullptr);
}

const Container* ZoneDriver::getContainer(const EntityID& id)
{
	for (const Container& c : _assets._csa.containers) {
		if (c.entityID == id) return &c;
	}
	return nullptr;
}

void ZoneDriver::setZone(const EntityID& cz, EntityID room)
{
	EntityID zone = cz;
	if (zone.empty()) {
		zone = _assets._csa.zones[0].entityID;
	}

	if (room.empty()) {
		const Zone& z = _assets.getZone(zone);
		const Room* r = z.firstRoom(_assets);
		if (!r) FatalError(fmt::format("Zone '{}' has no Room", z.name));
		assert(r);
		room = r->entityID;
	}

	_zone = _assets.getScriptRef(zone);
	_room = _assets.getScriptRef(room);
	assert(_zone.type == ScriptType::kZone);
	assert(_room.type == ScriptType::kRoom);
}

const Room& ZoneDriver::currentRoom() const
{
	return _assets._csa.rooms[_room.index];
}

const Zone& ZoneDriver::currentZone() const
{
	return _assets._csa.zones[_zone.index];
}

const Actor& ZoneDriver::getPlayer()
{
	return _assets.getActor("player");
}

std::vector<Edge> ZoneDriver::edges(EntityID room) const
{
	if (room.empty())
		room = _assets._csa.rooms[_room.index].entityID;

	std::vector<Edge> result;
	std::copy_if(_assets._csa.edges.begin(), _assets._csa.edges.end(), std::back_inserter(result), [&](const Edge& e) {
		return e.room1 == room || e.room2 == room;
	});
	return result;
}

std::vector<DirEdge> ZoneDriver::dirEdges(EntityID room) const
{
	if (room.empty())
		room = _assets._csa.rooms[_room.index].entityID;

	std::vector<DirEdge> result;
	for (const Edge& e : _assets._csa.edges) {
		bool found = false;
		bool flip = false;
		if (e.room1 == room) {
			found = true;
			flip = false;
		}
		else if (e.room2 == room) {
			found = true;
			flip = true;
		}
		if (!found) continue;

		DirEdge de;
		de.entityID = e.entityID;
		de.name = e.name;
		if (e.dir != Edge::Dir::kUnknown) {
			de.dir = flip ? (Edge::Dir)(((int)e.dir + 4) % 8) : e.dir;
			de.dirShort = Edge::dirToShortName(de.dir);
			de.dirLong = Edge::dirToLongName(de.dir);
		}
		de.dstRoom = flip ? e.room1 : e.room2;
		de.locked = mapData.coreData.coreBool(e.entityID, "locked", e.locked);
		de.key = e.key;
		result.push_back(de);
	}
	for (DirEdge& e : result) {
		size_t index = e.name.find("{dest}");
		if (index != std::string::npos) {
			const Room& dstRoom = _assets.getRoom(e.dstRoom);
			e.name.replace(index, 6, dstRoom.name);
		}
	}
	// Partition into indexed and direction. Sort both sides.
	std::partition(result.begin(), result.end(), [](const DirEdge& e) {
		return e.dir == Edge::Dir::kUnknown;
	});
	auto split = std::find_if(result.begin(), result.end(), [](const DirEdge& e) {
		return e.dir != Edge::Dir::kUnknown;
	});
	std::sort(result.begin(), split, [](const DirEdge& a, const DirEdge& b) {
		return a.name < b.name;
	});
	std::sort(split, result.end(), [](const DirEdge& a, const DirEdge& b) {
		return a.dir < b.dir;
	});
	return result;
}

const std::vector<EntityID>& ZoneDriver::entities(EntityID room) const
{
	if (room.empty())
		room = _assets._csa.rooms[_room.index].entityID;
	ScriptRef roomRef = _assets.getScriptRef(room);
	const Room& r = _assets._csa.rooms[roomRef.index];
	return r.objects;
}

ContainerVec ZoneDriver::getContainers(EntityID room)
{
	if (room.empty())
		room = _assets._csa.rooms[_room.index].entityID;

	const std::vector<EntityID>& eArr = this->entities(room);
	ContainerVec result;
	for (const auto& e : eArr) {
		ScriptRef ref = _assets.getScriptRef(e);
		if (ref.type == ScriptType::kContainer) {
			const Container& c = _assets._csa.containers[ref.index];
			bool eval = true;

			ScriptEnv env = { NO_ENTITY, zoneID(), roomID(), NO_ENTITY };
			ScriptHelper helper(_bridge, mapData.coreData, env);
			eval = helper.call(c.eval, 1);
\
			if (eval)
				result.push_back(&c);
		}
	}
	return result;
}

bool ZoneDriver::filterInteraction(const Interaction& i)
{
	bool eval = true;
	ScriptEnv env = { NO_ENTITY, zoneID(), roomID(), NO_ENTITY };
	ScriptHelper helper(_bridge, mapData.coreData, env);
	eval = helper.call(i.eval, 1);
	return eval;
}

InteractionVec ZoneDriver::getInteractions(EntityID room)
{
	if (room.empty())
		room = _assets._csa.rooms[_room.index].entityID;

	const std::vector<EntityID>& eArr = this->entities(room);
	InteractionVec result;
	for (const auto& e : eArr) {
		ScriptRef ref = _assets.getScriptRef(e);
		if (ref.type == ScriptType::kInteraction) {
			const Interaction* iact = &_assets._csa.interactions[ref.index];
			bool done = mapData.coreData.coreBool(iact->entityID, "done", false);
			if (iact->active(done) && filterInteraction(*iact)) {
				result.push_back(iact);
			}
		}
	}
	return result;
}

const Interaction* ZoneDriver::getRequiredInteraction()
{
	EntityID roomID = _assets._csa.rooms[_room.index].entityID;
	const std::vector<EntityID>& eArr = this->entities(roomID);
	for (const auto& e : eArr) {
		ScriptRef ref = _assets.getScriptRef(e);
		if (ref.type == ScriptType::kInteraction) {
			const Interaction* iact = &_assets._csa.interactions[ref.index];
			bool done = mapData.coreData.coreBool(iact->entityID, "done", false);
			if (iact->required && iact->active(done) && filterInteraction(*iact))
				return iact;
		}
	}
	return nullptr;
}

ScriptEnv ZoneDriver::getScriptEnv(const Interaction* interaction)
{
	ScriptEnv env;
	env.script = interaction->next;
	env.zone = currentZone().entityID;
	env.room = currentRoom().entityID;
	env.npc = interaction->npc;
	return env;
}

ScriptEnv ZoneDriver::getScriptEnv()
{
	ScriptEnv env;
	env.zone = currentZone().entityID;
	env.room = currentRoom().entityID;
	return env;
}

void ZoneDriver::markRequiredInteractionComplete(const Interaction* iact)
{
	if (iact) {
		ScriptRef ref = _assets.getScriptRef(iact->entityID);
		assert(ref.type == ScriptType::kInteraction);
		if (iact->required)
			mapData.coreData.coreSet(iact->entityID, "done", true, false);
	}
}

void ZoneDriver::markRequiredInteractionComplete(const EntityID& scriptID)
{
	EntityID roomID = _assets._csa.rooms[_room.index].entityID;
	const std::vector<EntityID>& eArr = this->entities(roomID);
	for (const auto& e : eArr) {
		ScriptRef ref = _assets.getScriptRef(e);
		if (ref.type == ScriptType::kInteraction) {
			const Interaction* iact = &_assets._csa.interactions[ref.index];
			if (iact->required && iact->next == scriptID) {
				mapData.coreData.coreSet(iact->entityID, "done", true, false);
			}
		}
	}
}


void ZoneDriver::teleport(const EntityID& room)
{
	// Note this can change the Room *and* the Zone.
	_room = _assets.getScriptRef(room);
	assert(_room.type == ScriptType::kRoom);
	
	const Zone& zone = _assets._csa.zones[_zone.index];

	for (size_t i = 0; i < zone.objects.size(); ++i) {
		if (zone.objects[i] == room) {
			// All good; in the same zone.
			return;
		}
	}

	// Need to change zones. Find the new zone.
	// FIXME: This is a a big slow linear search.
	for (size_t i = 0; i < _assets._csa.zones.size(); ++i) {
		const Zone& z = _assets._csa.zones[i];
		for (size_t j = 0; j < z.objects.size(); ++j) {
			if (z.objects[j] == room) {
				_zone = _assets.getScriptRef(z.entityID);
				return;
			}
		}
	}
	assert(false);	// We didn't find the zone.
}

/* ScriptCBHandler */
bool ZoneDriver::isLocked(const EntityID& id) const
{
	ScriptRef ref = _assets.getScriptRef(id);

	// Check the CoreData
	std::pair<bool, Variant> val = mapData.coreData.coreGet(id, "locked");
	if (val.first) return val.second.asBool();

	// Fallback to immutable:
	if (ref.type == ScriptType::kEdge) {
		return _assets._csa.edges[ref.index].locked;
	}
	else if (ref.type == ScriptType::kContainer) {
		return _assets._csa.containers[ref.index].locked;
	}
	return false;
}

/* ScriptCBHandler */
void ZoneDriver::setLocked(const EntityID& id, bool locked)
{
	ScriptRef ref = _assets.getScriptRef(id);
	if (ref.type == ScriptType::kEdge) {
		const Edge& e = _assets._csa.edges[ref.index];
		mapData.newsQueue.push(NewsItem::lock(locked, e, nullptr));
		//e.locked = locked;
		mapData.coreData.coreSet(e.entityID, "locked", locked, false);
	}
	else if (ref.type == ScriptType::kContainer) {
		const Container& c = _assets._csa.containers[ref.index];
		mapData.newsQueue.push(NewsItem::lock(locked, c, nullptr));
		//c.locked = locked;
		mapData.coreData.coreSet(c.entityID, "locked", locked, false);
	}
	else {
		assert(false);
	}
}

/* ScriptCBHandler */
void ZoneDriver::deltaItem(const EntityID& id, const EntityID& itemID, int n)
{
	const Item& item = _assets.getItem(itemID);
	if (_assets.inventories.find(id) == _assets.inventories.end()) {
		fmt::print("[LOG] Map::deltaItem (add/remove): invalid entity. entity={} item={} n={}\n", id, itemID, n);
		return;
	}
	Inventory& inv = _assets.inventories.at(id);
	inv.deltaItem(item, n);
}

/* ScriptCBHandler */
int ZoneDriver::numItems(const EntityID& id, const EntityID& itemID) const
{
	const Item& item = _assets.getItem(itemID);
	if (_assets.inventories.find(id) == _assets.inventories.end()) {
		fmt::print("[LOG] Map::numItems: invalid entity. entity={} item={}\n", id, itemID);
		return 0;
	}
	Inventory& inv = _assets.inventories.at(id);
	return inv.numItems(item);
}

void ZoneDriver::movePlayer(const EntityID& dst, bool tele)
{
	if (tele) teleport(dst);
	else move(dst);
}

void ZoneDriver::endGame(const std::string& msg, int bias)
{
	_endGameMsg = msg;
	_endGameBias = bias;
}

bool ZoneDriver::tryUnlock(const Entity& e)
{
	if (!isLocked(e.entityID))
		return true;

	std::string keyName;
	const Edge* edge = dynamic_cast<const Edge*>(&e);
	const Container* container = dynamic_cast<const Container*>(&e);

	if (edge)
		keyName = edge->key;
	else if (container)
		keyName = container->key;
	else
		return false;

	if (keyName.empty())
		return false;

	const Item& key = _assets.getItem(keyName);
	Inventory& inv = _assets.inventories.at("player");
	if (inv.hasItem(key)) {
		mapData.coreData.coreSet(e.entityID, "locked", false, false);
		if (edge)
			mapData.newsQueue.push(NewsItem::lock(false, *edge, &key));
		else if (container)
			mapData.newsQueue.push(NewsItem::lock(false, *container, &key));
		return true;
	}
	return false;
}

ZoneDriver::MoveResult ZoneDriver::move(const EntityID& roomEntityID)
{
	// Find an edge - brute force. May want to optimize. FIXME
	EntityID a = roomEntityID;
	EntityID b = _assets._csa.rooms[_room.index].entityID;

	const auto it = std::find_if(_assets._csa.edges.begin(), _assets._csa.edges.end(), [&](const Edge& e) {
		return (a == e.room1 && b == e.room2) || (a == e.room2 && b == e.room1);
		});

	if (it == _assets._csa.edges.end()) {
		FatalError(fmt::format("Can not find Room with entityID='{}'", roomEntityID));
	}

	assert(it != _assets._csa.edges.end());
	if (it == _assets._csa.edges.end())
		return MoveResult::kNoConnection;

	if (isLocked(*it))
		tryUnlock(*it);

	if (isLocked(*it))
		return MoveResult::kLocked;

	teleport(roomEntityID);
	checkScriptDriver();
	return MoveResult::kSuccess;
}

ZoneDriver::MoveResult ZoneDriver::move(const Edge& edge)
{
	EntityID current = _assets._csa.rooms[_room.index].entityID;
	if (edge.room1 == current)
		return move(edge.room2);
	else if (edge.room2 == current)
		return move(edge.room1);
	else
		return MoveResult::kNoConnection;
}

const EntityID& ZoneDriver::zoneID() const
{
	const Zone& zone = _assets._csa.zones[_zone.index];
	return zone.entityID;
}

const EntityID& ZoneDriver::roomID() const
{
	const Room& room = _assets._csa.rooms[_room.index];
	return room.entityID;
}

void ZoneDriver::save(std::ostream& stream) const
{
	mapData.coreData.save(stream);
	_assets.save(stream);
	ZoneDriver::saveTextRead(stream, mapData.textRead);

	fmt::print(stream, "Map = {{\n");
	//fmt::print(stream, "  playerID = '{}',\n", _playerID);
	fmt::print(stream, "  currentZone = '{}',\n", _assets._csa.zones[_zone.index].entityID);
	fmt::print(stream, "  currentRoom = '{}',\n", _assets._csa.rooms[_room.index].entityID);
	fmt::print(stream, "}}\n");

	if (_scriptDriver) {
		_scriptDriver->save(stream);
	}
}

EntityID ZoneDriver::load(ScriptBridge& loader)
{
	EntityID script;
	mapData.coreData.load(loader);
	_assets.load(loader);
	ZoneDriver::loadTextRead(loader, mapData.textRead);

	lua_State* L = loader.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	loader.pushGlobal("Map");
	EntityID room = loader.getStrField("currentRoom", {});
	EntityID zone = loader.getStrField("currentZone", {});
	_zone = _assets.getScriptRef(zone);
	_room = _assets.getScriptRef(room);
	lua_pop(L, 1);

	lua_getglobal(L, "ScriptEnv");
	if (!lua_isnil(L, -1)) {
		script = loader.getStrField("script", {});
		ScriptEnv env = this->getScriptEnv();
		env.script = script;
		_scriptDriver = new ScriptDriver(this->_assets, this->mapData, _bridge, env);
		_scriptDriver->load(loader);
	}
	lua_pop(L, 1);
	return script;
}

void ZoneDriver::saveTextRead(std::ostream& stream, const std::unordered_set<uint64_t>& text)
{
	int N = 4;
	fmt::print(stream, "TextRead = {{");
	int n = 0;
	for (uint64_t t : text) {
		if (n % N == 0) fmt::print(stream, "\n ");
		n++;
		fmt::print(stream, " {:#x},", t);
	}
	fmt::print(stream, "\n}}\n");
}

void ZoneDriver::loadTextRead(ScriptBridge& loader, std::unordered_set<uint64_t>& text)
{
	lua_State* L = loader.getLuaState();
	ScriptBridge::LuaStackCheck check(L);

	lua_getglobal(L, "TextRead");
	assert(lua_type(L, -1) == LUA_TTABLE);

	int i = 1;
	while (true) {
		lua_rawgeti(L, -1, i);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		uint64_t t = lua_tointeger(L, -1);
		lua_pop(L, 1);
		text.insert(t);
		i++;
	}
	lua_pop(L, 1);
}

ZoneDriver::Mode ZoneDriver::mode() const
{
	if (_scriptDriver) {
		assert(!_scriptDriver->done());
		ScriptType type = _scriptDriver->type();
		if (type == ScriptType::kText)
			return Mode::kText;
		else if (type == ScriptType::kChoices)
			return Mode::kChoices;
		else if (type == ScriptType::kBattle)
			return Mode::kBattle;
		else {
			assert(false);
			FATAL_INTERNAL_ERROR();
		}
	}
	return Mode::kNavigation;
}

void ZoneDriver::checkScriptDriver()
{
	if (_scriptDriver) {
		if (_scriptDriver->done()) {
			EntityID scriptID = _scriptDriver->env().script;
			markRequiredInteractionComplete(scriptID);
			delete _scriptDriver;
			_scriptDriver = nullptr;
		}
	}

	if (!_scriptDriver) {
		const Interaction* iact = getRequiredInteraction();
		if (iact) {
			ScriptEnv env = getScriptEnv(iact);
			_scriptDriver = new ScriptDriver(this->_assets, this->mapData, _bridge, env, iact->code);
		}
	}
}

void ZoneDriver::startInteraction(const Interaction* interaction)
{
	assert(_scriptDriver == nullptr);
	ScriptEnv env = getScriptEnv(interaction);
	_scriptDriver = new ScriptDriver(this->_assets, this->mapData, _bridge, env, interaction->code);
	checkScriptDriver();
}


TextLine ZoneDriver::text() const
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kText);
	return _scriptDriver->line();
}

void ZoneDriver::advance()
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kText);
	_scriptDriver->advance();
	checkScriptDriver();
}

const Choices& ZoneDriver::choices() const
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kChoices);
	return _scriptDriver->choices();
}

void ZoneDriver::choose(int n)
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kChoices);
	_scriptDriver->choose(n);
	checkScriptDriver();
}

const Battle& ZoneDriver::battle() const
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kBattle);
	return _scriptDriver->battle();
}

VarBinder ZoneDriver::battleVarBinder() const
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kBattle);
	return _scriptDriver->varBinder();
}

void ZoneDriver::battleDone()
{
	assert(_scriptDriver);
	assert(_scriptDriver->type() == ScriptType::kBattle);
	_scriptDriver->advance();
	checkScriptDriver();
}

ZoneDriver::TransferResult ZoneDriver::transferAll(const EntityID& srcEntity, const EntityID& dstEntity) {
	CHECK(_assets.isAsset(srcEntity));
	CHECK(_assets.isAsset(dstEntity));

	if (isLocked(srcEntity))
		tryUnlock(srcEntity);

	if (isLocked(srcEntity) || isLocked(dstEntity))
		return TransferResult::kLocked;

	Inventory& src = _assets.getInventory(*_assets.get(srcEntity));
	while (!src.emtpy()) {
		this->transfer(*src.items()[0].pItem, srcEntity, dstEntity, INT_MAX);
	}
	return TransferResult::kSuccess;
}

ZoneDriver::TransferResult ZoneDriver::transfer(const Item& item, const EntityID& srcEntity, const EntityID& dstEntity, int n) {
	if (isLocked(srcEntity) || isLocked(dstEntity))
		return TransferResult::kLocked;

	Inventory& src = _assets.getInventory(*_assets.get(srcEntity));
	Inventory& dst = _assets.getInventory(*_assets.get(dstEntity));
	int delta = src.numItems(item);
	lurp::Inventory::transfer(item, src, dst, n);
	int count = dst.numItems(item);

	const EntityID& playerID = getPlayer().entityID;
	if (srcEntity == playerID || dstEntity == playerID) {
		mapData.newsQueue.push(NewsItem::itemDelta(item, delta, count));
	}
	return TransferResult::kSuccess;
}

} // namespace lurp
