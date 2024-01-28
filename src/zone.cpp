#include "zone.h"
#include "scriptasset.h"
#include "scriptbridge.h"
#include "mapdata.h"

#include <fmt/core.h>
#include <fmt/ostream.h>

const Room* Zone::firstRoom(const ScriptAssets& assets) const
{
	for (const EntityID& obj : objects) {
		ScriptRef ref = assets.get(obj);
		if (ref.type == ScriptType::kRoom) {
			return &assets.rooms[ref.index];
		}
	}
	return nullptr;
}

/*static*/ std::string Edge::dirToShortName(Dir dir)
{
	switch (dir) {
	case Dir::kNorth: return "N";
	case Dir::kNortheast: return "NE";
	case Dir::kEast: return "E";
	case Dir::kSoutheast: return "SE";
	case Dir::kSouth: return "S";
	case Dir::kSouthwest: return "SW";
	case Dir::kWest: return "W";
	case Dir::kNorthwest: return "NW";
	default: return "";
	}
}

/*static*/ std::string Edge::dirToLongName(Dir dir)
{
	switch (dir) {
	case Dir::kNorth: return "North";
	case Dir::kNortheast: return "Northeast";
	case Dir::kEast: return "East";
	case Dir::kSoutheast: return "Southeast";
	case Dir::kSouth: return "South";
	case Dir::kSouthwest: return "Southwest";
	case Dir::kWest: return "West";
	case Dir::kNorthwest: return "Northwest";
	default: return "";
	}
}
