#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace lurp {

class LuaBridge;

class BoardDriver {
public:
	struct Cell {
		std::string name;
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		std::vector<int> connections;
	};

	enum class Color {
		defaultColor,	// needs to be zero
		red,
		orange,
		yellow,
		green,
		blue,
		purple,
		white,
	};

	struct Counter {
		std::string name;				// note this is the value on the player class: player.gold -> name = "gold"
		int value = 0;
		int min = 0;
		int max = 0;
		int inc = 0;
	};

	struct Player {
		// Reflection of Lua:
		int index = 0;					// 0 is the first player in C++, 1 is the first in Lua 
		std::vector<int> meepleUIDs;
		std::vector<Counter> counters;
	};

	struct Meeple {
		// Reflection of Lua:
		std::string name;
		std::string label;
		std::string pos;
		int uid = 0;
		Color color;

		// Convenience! If owned by a player, this is the player number.
		// 0 is the first player.
		int player = -1;
	};

	struct Move {
		int player = 0;
		Meeple meeple;
		const Cell* from = nullptr;
		const Cell* to = nullptr;
	};

	BoardDriver(LuaBridge& bridge) : bridge(bridge) {}

	bool started() const { return _started; }

	void loadBoard();
	void createGameBox();
	void setupGame();
	void initGame();

	void load(const std::filesystem::path& p);
	void save(const std::filesystem::path& p);

	static Color toColor(const std::string&);

	Player queryPlayer(int player) const;
	std::vector<Player> queryPlayers() const;
	std::vector<Meeple> queryAllMeeples() const;
	std::vector<Meeple> queryMeeplesOnBoard() const;

	const std::vector<Cell>& board() const { return _board; }
	const Cell* getCell(const std::string& name) const;

	bool done() const;
	int nPlayers() const { return _numPlayers; }
	void move(const Move& move);

	std::vector<Move> queryMoves(int player) const;

private:

	LuaBridge& bridge;
	std::vector<Cell> _board;
	int _maxPlayers = 0;
	int _numPlayers = 0;		// fixme: this should go away
	bool _started = false;

	void parseBoardTable();
};

} // namespace lurp