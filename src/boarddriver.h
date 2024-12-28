#pragma once

#include <string>
#include <vector>

namespace lurp {

class LuaBridge;

class BoardDriver {
public:
	BoardDriver(LuaBridge& bridge) : bridge(bridge) {}

	void loadBoard();
	void createGameBox();
	void setupGame();

	struct Cell {
		std::string name;
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		std::vector<int> connections;
	};

	enum class Color {
		red,
		orange,
		yellow,
		green,
		blue,
		purple,
		white,
	};

	struct Meeple {
		std::string name;
		std::string location;
		Color color;
	};

	std::vector<Meeple> queryMeeplesOnBoard();
	const std::vector<Cell>& board() const { return _board; }
private:

	LuaBridge& bridge;
	std::vector<Cell> _board;
	int64_t _maxPlayers = 0;

	void parseBoardTable();
};

} // namespace lurp