#include "consoleboard.h"
#include "luabridge.h"
#include "boarddriver.h"

#include <fmt/core.h>

using namespace lurp;

static void drawLine(uint16_t* buffer, int w, int h, int x0, int y0, int x1, int y1, char c)
{
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx - dy;
	while (true) {
		assert(x0 >= 0 && x0 < w);
		assert(y0 >= 0 && y0 < h);
		buffer[y0 * w + x0] = uint16_t(c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

static std::string renderBoard(const BoardDriver& driver)
{
	int width = 0;
	int height = 0;

	const std::vector<BoardDriver::Cell>& boardCells = driver.board();
	std::vector<BoardDriver::Meeple> meeples = driver.queryMeeplesOnBoard();

	for (const auto& cell : boardCells) {
		width = std::max(width, cell.x + cell.w);
		height = std::max(height, cell.y + cell.h);
	}
	uint16_t* board = new uint16_t[width * height];
	memset(board, 0, width * height * sizeof(board[0]));

	for (const auto& cell : boardCells) {
		int cx = cell.x + cell.w / 2;
		int cy = cell.y + cell.h / 2;

		// draw the connections.
		for (int c : cell.connections) {
			int dx = boardCells[c].x + boardCells[c].w / 2;
			int dy = boardCells[c].y + boardCells[c].h / 2;

			drawLine(board, width, height, cx, cy, dx, dy, '*');
		}
	}
	for (const auto& cell : boardCells) {
		// draw the cell.
		for (int y = cell.y; y < cell.y + cell.h; y++) {
			drawLine(board, width, height, cell.x, y, cell.x + cell.w - 1, y, ' ');
		}
		drawLine(board, width, height, cell.x, cell.y, cell.x, cell.y + cell.h - 1, '[');
		drawLine(board, width, height, cell.x + cell.w - 1, cell.y, cell.x + cell.w - 1, cell.y + cell.h - 1, ']');
	}
	for (const auto& cell : boardCells) {
		// draw the name.
		for (size_t i = 0; i < cell.name.size(); i++) {
			board[cell.y * width + cell.x + cell.w / 2 - cell.name.size() / 2 + i] = cell.name[i];
		}
	}

	//drawLine(board, width, height, 1, 1, 10, 5, '*');

	std::string result;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8_t c = int8_t(board[y * width + x]);
			result += c ? c : ' ';
		}
		result += '\n';
	}

	delete[] board;
	return result;
}

void ConsoleBoardDriver(const std::string& gameFile, const std::string& gameName)
{
	// Loop:
	//    - Print the board
	//    - Get user input
	//    - Move the player

	// Should the representation of the board be here or in the script code?

	LuaBridge bridge;	// Problem #1: There's a bunch of game specific code in the scriptbridge
						//             Create the luaBridge
	bridge.loadLUA(gameFile.c_str(), "_board.lua");	// Load the game

	BoardDriver driver(bridge);
	driver.loadBoard();
	driver.createGameBox();
	driver.setupGame();

	std::string b = renderBoard(driver);
	fmt::print("{}", b);

#if 0
	// - KISS: start with a graph based board.
	// - Need a representation here to put on the pieces.
	// - Multi player (sigh - connections are a pain. start w/local network)

	loadBoard();
	setupBoard();

	while(true) {
		playerMoves();
		mechMoves();
	}
#endif
}