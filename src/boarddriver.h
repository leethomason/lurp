#pragma once

#include <string>
#include <vector>

namespace lurp {

class LuaBridge;

class BoardDriver {
public:
	BoardDriver(LuaBridge& bridge) : bridge(bridge) {}

	void loadBoard();

private:
	struct BoardCell {
		std::string name;
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		std::vector<int> connections;
	};

	LuaBridge& bridge;
	std::vector<BoardCell> _board;

	void parseBoardTable();
	std::string renderBoard();
	void drawLine(uint8_t* buffer, int w, int h, int x0, int y0, int x1, int y1, char c);
};

} // namespace lurp