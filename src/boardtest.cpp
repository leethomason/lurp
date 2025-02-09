#include "boardtest.h"
#include "test.h"
#include "boarddriver.h"
#include "luabridge.h"

using namespace lurp;

static void BasicTest()
{
	LuaBridge bridge;
	bridge.loadLUA("game/board/board.lua", "_board.lua");
	BoardDriver driver(bridge);

	driver.loadBoard();
	driver.createGameBox();
	driver.setupGame();
	driver.initGame();

	TEST(driver.nPlayers() == 2);
	TEST(driver.currentMove() == 0);
}


void RunBoardTests()
{
	RUN_TEST(BasicTest());
}
