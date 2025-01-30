#include "consoleboard.h"
#include "luabridge.h"
#include "boarddriver.h"
#include "util.h"
#include "consoleutil.h"
#include "../platform.h"

#include <ionic/ionic.h>

#include <fmt/core.h>
#include <numeric>

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

	// Draw the meeples
	using Meeple = BoardDriver::Meeple;
	std::vector<Meeple> meeples = driver.queryMeeplesOnBoard();
	for (const auto& cell : boardCells) {
		std::vector<Meeple> mHere = filter(meeples, [name = cell.name](const Meeple& m) {return m.pos == name; });
		if (mHere.empty())
			continue;

		size_t textLen = reduce(mHere, (size_t)0, [](size_t sum, const Meeple& m) { return sum + m.label.size(); });
		REQUIRE(textLen > 0);
		textLen += mHere.size() - 1; // spaces between names

		int x = cell.x + cell.w / 2 - int(textLen) / 2;
		int y = cell.y + 1;
		for (const auto& m : mHere) {
			for (size_t i = 0; i < m.label.size(); i++) {
				board[y * width + x + i] = m.label[i] | (uint16_t(m.color) << 8);
			}
			x += int(m.label.size()) + 1;
		}
	}

	std::string result;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint16_t c16 = board[y * width + x];
			char c = (c16 & 0xff) ? char(c16 & 0xff) : ' ';

			using Color = BoardDriver::Color;
			Color color = Color(c16 >> 8);

			switch (color) {
			case Color::defaultColor: result += c; break;
			case Color::red: result += ionic::Table::colorize(ionic::Color::red, std::string(1, c)); break;
			case Color::orange: result += ionic::Table::colorize(ionic::Color::brightYellow, std::string(1, c)); break;
			case Color::yellow: result += ionic::Table::colorize(ionic::Color::yellow, std::string(1, c)); break;
			case Color::green: result += ionic::Table::colorize(ionic::Color::green, std::string(1, c)); break;
			case Color::blue: result += ionic::Table::colorize(ionic::Color::brightBlue, std::string(1, c)); break;
			case Color::purple: result += ionic::Table::colorize(ionic::Color::magenta, std::string(1, c)); break;
			case Color::white: result += ionic::Table::colorize(ionic::Color::white, std::string(1, c)); break;
			default:
				assert(false);
			}
		}
		result += '\n';
	}

	delete[] board;
	return result;
}

void PrintPlayer(const BoardDriver& driver, int playerIndex)
{
	using Player = BoardDriver::Player;

	fmt::print("Player {}:\n", playerIndex + 1);
	Player p = driver.queryPlayer(playerIndex);
	for (const auto& c : p.counters) {
		fmt::print("  {}: {} ({} - {})\n", c.name, c.value, c.min, c.max);
	}
}

void ConsoleBoardDriver(const std::string& gameFile, const std::string& gameName)
{
	// Loop:
	//    - Print the board
	//    - Get user input
	//    - Move the player

	// Should the representation of the board be here or in the script code?

	LuaBridge bridge;
	bridge.loadLUA(gameFile.c_str(), "_board.lua");
	BoardDriver driver(bridge);

	// Load a file if present & desired.
	{
		std::filesystem::path path = SavePath(gameName, "autosave");
		if (std::filesystem::exists(path)) {
			fmt::print("Load (y/n)>> ");
			std::string input = ReadString();
			if (input == "y") {
				driver.loadBoard();
				driver.load(path);
			}
		}
	}
	// If not loaded, start a new game.
	if (!driver.started()) {
		driver.loadBoard();
		driver.createGameBox();
		driver.setupGame();
	}
	driver.initGame();

	//std::vector<BoardDriver::Move> moves = driver.queryMoves(0);
	//for (const auto& m : moves) {
	//	fmt::print("Player {} meeple {} from {} to {}\n", 0, m.meeple.label, m.from->name, m.to->name);
	//}

	bool save = false;

	while (!driver.done() && !save) {
		std::string b = renderBoard(driver);
		fmt::print("{}", b);

		bool moveEntered = false;
		REQUIRE(driver.nPlayers() > 0);



		for (int i = 0; i < driver.nPlayers(); i++) {
			std::vector<BoardDriver::Move> moves = driver.queryMoves(i);

			//fmt::print("Player {}'s turn\n", i);
			PrintPlayer(driver, i);
			
			int index = 1;
			fmt::print("\n0: End turn\n");
			for (const auto& m : moves) {
				fmt::print("{}: Meeple {} from {} to {}\n", index++, m.meeple.label, m.from->name, m.to->name);
			}

			while (true) {
				fmt::print(">> ");
				std::string input = ReadString();
				if (input == "/s") {
					save = true;
					if (moveEntered)
						break;
					else
						continue;
				}
				else if (input == "/q") {
					return;
				}

				Value v = Value::ParseValue(input);

				if (v.type == Value::Type::kInt && v.intInRange((int)(moves.size() + 1))) {
					if (v.intVal == 0)
						break;
					moveEntered = true;
					driver.move(moves[v.intVal - 1]);
					break;
				}
				else {
					fmt::print("Invalid move\n");
				}
			}
		}
		if (save) {
			std::filesystem::path path = SavePath(gameName, "autosave");
			driver.save(path);
		}
	}
}
