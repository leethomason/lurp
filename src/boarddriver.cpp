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
	std::string b = renderBoard();
	fmt::print("{}", b);

	bridge.pop(nResults);
}

void BoardDriver::fillBox()
{
	bridge.pushGlobal("onSetupBox");
	bridge.pushGlobal("Box");
	bridge.pCallFunc(1, 0);
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

std::string BoardDriver::renderBoard()
{
	int width = 0;
	int height = 0;
	for (const auto& cell : _board) {
		width = std::max(width, cell.x + cell.w);
		height = std::max(height, cell.y + cell.h);
	}
	uint8_t* board = new uint8_t[width * height];
	memset(board, 0, width * height);

	for (const auto& cell : _board) {
		int cx = cell.x + cell.w / 2;
		int cy = cell.y + cell.h / 2;

		// draw the connections.
		for (int c : cell.connections) {
			int dx = _board[c].x + _board[c].w / 2;
			int dy = _board[c].y + _board[c].h / 2;

			drawLine(board, width, height, cx, cy, dx, dy, '*');
		}
	}
	for (const auto& cell : _board) {
		// draw the cell.
		for (int y = cell.y; y < cell.y + cell.h; y++) {
			drawLine(board, width, height, cell.x, y, cell.x + cell.w - 1, y, ' ');
		}
		drawLine(board, width, height, cell.x, cell.y, cell.x, cell.y + cell.h - 1, '[');
		drawLine(board, width, height, cell.x + cell.w - 1, cell.y, cell.x + cell.w - 1, cell.y + cell.h - 1, ']');
	}
	for (const auto& cell : _board) {
		// draw the name.
		for (size_t i = 0; i < cell.name.size(); i++) {
			board[cell.y * width + cell.x + cell.w / 2 - cell.name.size() / 2 + i] = cell.name[i];
		}
	}

	//drawLine(board, width, height, 1, 1, 10, 5, '*');

	std::string result;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint8_t c = board[y * width + x];
			result += c ? c : ' ';
		}
		result += '\n';
	}

	delete[] board;
	return result;
}

void BoardDriver::drawLine(uint8_t* buffer, int w, int h, int x0, int y0, int x1, int y1, char c)
{
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx - dy;
	while (true) {
		assert(x0 >= 0 && x0 < w);
		assert(y0 >= 0 && y0 < h);
		buffer[y0 * w + x0] = uint8_t(c);
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

} // namespace lurp


