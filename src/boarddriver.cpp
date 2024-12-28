#include "boarddriver.h"
#include "luabridge.h"
#include "debug.h"

#include <fmt/core.h>

namespace lurp {

void BoardDriver::loadBoard()
{
	std::vector<Variant> args;
	int nResults = bridge.callGlobalFunc("onFetchBoard", args);
	
	REQUIRE(nResults == 1);
	REQUIRE(bridge.isTable(-1));
	parseBoardTable();
	bridge.pop(nResults);
}

void BoardDriver::createGameBox()
{
	bridge.pushGlobal("onSetupBox");
	bridge.pushGlobal("Box");
	bridge.pCallFunc(1, 1);
	_maxPlayers = bridge.toInt(-1);
	bridge.pop();
	REQUIRE(_maxPlayers > 0);
}

void BoardDriver::setupGame()
{
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
	for (TableIt it(bridge.getLuaState(), -1); !it.done(); it.next()) {
		REQUIRE(it.vType() == LUA_TTABLE);
		REQUIRE(bridge.hasField("name"));

		BoardCell cell;
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
			auto dst = std::find_if(_board.begin(), _board.end(), [name](const BoardCell& cell) {
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


} // namespace lurp


