#pragma once

#include <string>
#include <vector>

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

	struct Player {
		// Reflection of Lua:
		int index = 0;					// 0 is the first player in C++, 1 is the first in Lua 
		std::vector<int> meepleUIDs;
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

	BoardDriver(LuaBridge& bridge) : bridge(bridge) {}

	void loadBoard();
	void createGameBox();
	void setupGame();

	static Color toColor(const std::string&);

	std::vector<Player> queryPlayers() const;
	std::vector<Meeple> queryAllMeeples() const;
	std::vector<Meeple> queryMeeplesOnBoard() const;

	const std::vector<Cell>& board() const { return _board; }
	const Cell* getCell(const std::string& name) const;

	bool done() const { return false; }	// FIXME
	int nPlayers() const { return _numPlayers; }

	struct Move {
		int player = 0;
		const Meeple* meeple = nullptr;
		const Cell* from = nullptr;
		const Cell* to = nullptr;
	};
	std::vector<Move> queryMoves(int player) const;

private:

	LuaBridge& bridge;
	std::vector<Cell> _board;
	int _maxPlayers = 0;
	int _numPlayers = 0;

	void parseBoardTable();
};

} // namespace lurp