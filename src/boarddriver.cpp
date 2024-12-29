#include "boarddriver.h"
#include "luabridge.h"
#include "debug.h"

#include <fmt/core.h>

namespace lurp {

void BoardDriver::loadBoard()
{
	LuaStackCheck check(bridge.getLuaState());

	std::vector<Variant> args;
	int nResults = bridge.callGlobalFunc("onFetchBoard", args);
	
	REQUIRE(nResults == 1);
	REQUIRE(bridge.isTable(-1));
	parseBoardTable();
	bridge.pop(nResults);
}

void BoardDriver::createGameBox()
{
	LuaStackCheck check(bridge.getLuaState());

	bridge.pushGlobal("onSetupBox");
	bridge.pushGlobal("Box");
	bridge.pCallFunc(1, 1);
	_maxPlayers = bridge.toInt(-1);
	bridge.pop();
	REQUIRE(_maxPlayers > 0);
}

void BoardDriver::setupGame()
{
	LuaStackCheck check(bridge.getLuaState());

	bridge.pushGlobal("_createPlayers");
	bridge.pushInt(_maxPlayers);
	bridge.pCallFunc(1, 0);

	bridge.pushGlobal("onSetupGame");
	bridge.pushGlobal("Players");
	bridge.pushGlobal("Box");
	bridge.pCallFunc(2, 0);
}

void BoardDriver::parseBoardTable()
{
	LuaStackCheck check(bridge.getLuaState());

	for (TableIt it(bridge.getLuaState(), -1); !it.done(); it.next()) {
		REQUIRE(it.vType() == LUA_TTABLE);
		REQUIRE(bridge.hasField("name"));

		Cell cell;
		cell.name = bridge.getStrField("name", {});
		cell.x = bridge.getIntField("x", 0);
		cell.y = bridge.getIntField("y", 0);
		cell.w = bridge.getIntField("w", 1);
		cell.h = bridge.getIntField("h", 1);
		_board.push_back(cell);
	}

	int index = 0;
	for (TableIt it(bridge.getLuaState(), -1); !it.done(); it.next(), index++) {
		if (!bridge.hasField("connect")) {
			continue;
		}
		bridge.pushTable("connect");

		for (TableIt c(bridge.getLuaState(), -1); !c.done(); c.next()) {
			REQUIRE(c.vType() == LUA_TSTRING);
			const std::string name = c.value().str;
			auto dst = std::find_if(_board.begin(), _board.end(), [name](const Cell& cell) {
				return cell.name == name;
				});
			REQUIRE(dst != _board.end());
			_board[index].connections.push_back(int(std::distance(_board.begin(), dst)));
		}

		bridge.pop();
	}

	for (const auto& cell : _board) {
		fmt::print("{:<20} {},{} {}x{}\n", cell.name.c_str(), cell.x, cell.y, cell.w, cell.h);
		for (int c : cell.connections) {
			fmt::print("  -> {}\n", _board[c].name.c_str());
		}
	}
}

BoardDriver::Color BoardDriver::toColor(const std::string& s)
{
	if (s == "red") return Color::red;
	if (s == "orange") return Color::orange;
	if (s == "yellow") return Color::yellow;
	if (s == "green") return Color::green;
	if (s == "blue") return Color::blue;
	if (s == "purple") return Color::purple;
	if (s == "white") return Color::white;
	return Color::defaultColor;
}

const BoardDriver::Cell* BoardDriver::getCell(const std::string& location) const
{
	auto it = std::find_if(_board.begin(), _board.end(), [location](const Cell& c) {
		return c.name == location;
		});
	if (it == _board.end())
		return nullptr;
	return &(*it);
}

std::vector<BoardDriver::Meeple> BoardDriver::queryMeeplesOnBoard() const
{
	std::vector<Meeple> meeples;
	LuaStackCheck check(bridge.getLuaState());

	// Need to look at Box.meeples
	bridge.pushGlobal("Box");
	REQUIRE(bridge.isTable(-1));
	bridge.pushTable("meeples");
	REQUIRE(bridge.isTable(-1));

	for (TableIt it(bridge.getLuaState()); !it.done(); it.next()) {
		Meeple m;
		m.name = bridge.getStrField("id", {});
		m.location = bridge.getStrField("pos", { "" });
		std::string color = bridge.getStrField("color", { "white" });
		m.color = toColor(color);

		const Cell* cell = getCell(m.location);
		if (cell) {
			meeples.push_back(m);
		}
	}

	bridge.pop(2);
	return meeples;
}

} // namespace lurp


