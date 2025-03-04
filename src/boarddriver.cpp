#include "boarddriver.h"
#include "luabridge.h"
#include "debug.h"
#include "util.h"

#include <fmt/core.h>
#include <fstream>

namespace lurp {

void BoardDriver::load(const std::filesystem::path& p)
{
	LuaStackCheck check(bridge.getLuaState());

	bridge.doFile(p.string());
	REQUIRE(bridge.isTable(-1));
	{
		// Awkward function calls.
		bridge.pushGlobal("_deserialize");
		REQUIRE(bridge.isFunc(-1));
		bridge.dup(-2);
		bridge.pCallFunc(1, 0);
		bridge.pop(1);
	}

	_started = true;

	bridge.pushGlobal("Players");
	_numPlayers = bridge.getLen();
	bridge.pop();
}

void BoardDriver::save(const std::filesystem::path& p)
{
	std::ofstream fp(p);
	if (!fp.is_open()) {
		std::string msg = fmt::format("Could not open file '{}' for writing", p.string());
		FatalError(msg);
	}

	LuaStackCheck check(bridge.getLuaState());
	int nRet = bridge.callGlobalFunc("_serialize", {});
	REQUIRE(nRet == 1);
	REQUIRE(bridge.isString(-1));
	std::string s = bridge.toString(-1);
	bridge.pop();

	fp << s << "\n";
}

void BoardDriver::loadBoard()
{
	LuaStackCheck check(bridge.getLuaState());

	std::vector<Variant> args;
	int nResults = bridge.callGlobalFunc("_onFetchBoard", args);
	
	REQUIRE(nResults == 1);
	REQUIRE(bridge.isTable(-1));
	parseBoardTable();
	
	bridge.pop(nResults);
}

void BoardDriver::createGameBox()
{
	REQUIRE(!_started);
	_started = true;

	LuaStackCheck check(bridge.getLuaState());

	bridge.pushGlobal("_onSetupBox");
	bridge.pCallFunc(0, 1);
	_maxPlayers = (int)bridge.toInt(-1);
	bridge.pop();
	REQUIRE(_maxPlayers > 0);
}

void BoardDriver::setupGame()
{
	LuaStackCheck check(bridge.getLuaState());

	bridge.pushGlobal("_createPlayers");
	// FIXME: need to set the number of players correctly.
	_numPlayers = std::min(2, int(_maxPlayers));
	bridge.pushInt(_numPlayers);
	bridge.pCallFunc(1, 0);

	bridge.pushGlobal("_onSetupGame");
	bridge.pCallFunc(0, 0);
}

void BoardDriver::initGame()
{
	LuaStackCheck check(bridge.getLuaState());
	bridge.pushGlobal("_onInit");
	bridge.pCallFunc(0, 0);
}

void BoardDriver::nextTurn()
{
	LuaStackCheck check(bridge.getLuaState());
	bridge.pushGlobal("_nextTurn");
	bridge.pCallFunc(0, 0);
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

BoardDriver::Player BoardDriver::queryPlayer(int player) const
{
	LuaStackCheck check(bridge.getLuaState());

	bridge.pushGlobal("Players");
	REQUIRE(bridge.isTable(-1));
	bridge.pushTable("", player + 1);
	REQUIRE(bridge.isTable(-1));

	Player p;
	p.index = player;

	// Meeples
	bridge.pushTable("meeples");
	REQUIRE(bridge.isTable(-1));

	for (TableIt m(bridge.getLuaState()); !m.done(); m.next()) {
		int64_t uid = bridge.getIntField("uid", {});
		p.meepleUIDs.push_back((int)uid);
	}

	bridge.pop();

	// Counters
	// Tricky - these are named, not indexed.

	for (TableIt c(bridge.getLuaState()); !c.done(); c.next()) {
		if (c.key().type != LUA_TSTRING)
			continue;
		if (bridge.typeOf(-1) != LUA_TTABLE)
			continue;
		// Look for 'incValue' as a field.
		if (!bridge.hasField("incValue"))
			continue;

		Counter counter;
		counter.name = c.key().str;
		counter.value = bridge.getIntField("value", 0);
		counter.min = bridge.getIntField("min", 0);
		counter.max = bridge.getIntField("max", 0);
		counter.inc = bridge.getIntField("incValue", 0);
		p.counters.push_back(counter);
	}

	// Clean up.
	bridge.pop(2);
	return p;
}

std::vector<BoardDriver::Player> BoardDriver::queryPlayers() const
{
	std::vector<Player> players;
	LuaStackCheck check(bridge.getLuaState());

	for (int i = 0; i < _numPlayers; i++) {
		players.push_back(queryPlayer(i));
	}

	return players;
}

std::vector<BoardDriver::Meeple> BoardDriver::queryAllMeeples() const
{
	std::vector<Meeple> meeples;
	LuaStackCheck check(bridge.getLuaState());
	std::vector<Player> players = queryPlayers();

	// Need to look at Box.meeples
	bridge.pushGlobal("Box");
	REQUIRE(bridge.isTable(-1));
	bridge.pushTable("meeples");
	REQUIRE(bridge.isTable(-1));

	for (TableIt it(bridge.getLuaState()); !it.done(); it.next()) {
		Meeple m;
		m.name = bridge.getStrField("name", {});
		m.label = bridge.getStrField("label", { "M" });
		m.pos = bridge.getStrField("pos", { "" });
		m.uid = bridge.getIntField("uid", { 0 });

		std::string color = bridge.getStrField("color", { "white" });
		m.color = toColor(color);

		// For convenience, if owned by a player, this is the player number.
		for (size_t i = 0; i < players.size(); i++) {
			if (std::find(players[i].meepleUIDs.begin(), players[i].meepleUIDs.end(), m.uid) != players[i].meepleUIDs.end()) {
				m.player = (int)i;
				break;
			}
		}
		meeples.push_back(m);
	}
	bridge.pop(2);
	return meeples;
}

std::vector<BoardDriver::Meeple> BoardDriver::queryMeeplesOnBoard() const
{
	std::vector<Meeple> meeples = queryAllMeeples();
	return filter(meeples, [](const Meeple& m) { return !m.pos.empty(); });
}

std::vector<BoardDriver::Move> BoardDriver::queryMoves(int player) const
{
	std::vector<Move> moves;

	// Meeples are what moves (not the player)
	const std::vector<Meeple> allMeeples = queryMeeplesOnBoard();
	const std::vector<Meeple> meeples = filter(allMeeples, [player](const Meeple& m) { return m.player == player; });

	// Just supporting adjacent moves for now.
	for (const auto& m : meeples) {
		const Cell* cell = getCell(m.pos);
		REQUIRE(cell != nullptr);
		for (int c : cell->connections) {
			const Cell* dst = &_board[c];

			Move move;
			move.player = player;
			move.meeple = m;
			move.from = cell;
			move.to = dst;

			// isMoveAllowed(game, player, meeple, start, dst)
			int nRet = 0;
			bridge.pushGlobal("isMoveAllowed");											// function ref
			bridge.pushGlobal("Game");
			nRet = bridge.callGlobalFunc("_queryPlayerFromIndex", { player + 1 } );		// player table (one based)
			REQUIRE(nRet == 1 && bridge.isTable(-1));
			nRet = bridge.callGlobalFunc("_queryMeepleFromUID", { m.uid });				// meeple table
			REQUIRE(nRet == 1 && bridge.isTable(-1));
			nRet = bridge.callGlobalFunc("_queryCellFromName", { m.pos });				// cell table
			REQUIRE(nRet == 1 && bridge.isTable(-1));
			nRet = bridge.callGlobalFunc("_queryCellFromName", { dst->name });			// cell table
			REQUIRE(nRet == 1 && bridge.isTable(-1));

			bridge.pCallFunc(5, 1);
			if (bridge.toBool(-1)) {
				moves.push_back(move);
			}
			bridge.pop();
		}
	}
	return moves;
}

void BoardDriver::move(const BoardDriver::Move& move)
{
	LuaStackCheck check(bridge.getLuaState());

	// Do the move.
	bridge.callGlobalFunc("_queryMeepleFromUID", { move.meeple.uid });				// meeple table
	bridge.setStrField("pos", move.to->name);
	bridge.pop();

	// onMeepleMoved(game, player, meeple, start, dst)
	int nRet = 0;
	bridge.pushGlobal("onMeepleMoved");	
	bridge.pushGlobal("Game");
	nRet = bridge.callGlobalFunc("_queryPlayerFromIndex", { move.player + 1 } );	// player table (one based)
	REQUIRE(nRet == 1 && bridge.isTable(-1));
	nRet = bridge.callGlobalFunc("_queryMeepleFromUID", { move.meeple.uid });		// meeple table
	REQUIRE(nRet == 1);
	nRet = bridge.callGlobalFunc("_queryCellFromName", { move.meeple.pos });		// cell table
	REQUIRE(nRet == 1);
	nRet = bridge.callGlobalFunc("_queryCellFromName", { move.to->name });			// cell table
	REQUIRE(nRet == 1);

	bridge.pCallFunc(5, 0);
}

bool BoardDriver::done() const
{
	LuaStackCheck check(bridge.getLuaState());

	int nRet = bridge.callGlobalFunc("isGameOver", {});
	REQUIRE(nRet == 1);
	bool gameOver = bridge.toBool(-1);
	bridge.pop();
	return gameOver;
}

int BoardDriver::currentMove() const
{
	LuaStackCheck check(bridge.getLuaState());
	int nRet = bridge.callGlobalFunc("_queryCurrentPlayerIndex", {});
	REQUIRE(nRet == 1);
	int player = (int)bridge.toInt(-1) - 1;
	bridge.pop();
	return player;
}

} // namespace lurp


