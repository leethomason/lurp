#include "consoleboard.h"
#include "luabridge.h"
#include "boarddriver.h"

using namespace lurp;

void ConsoleBoardDriver(const std::string& gameFile, const std::string& gameName)
{
	// Loop:
	//    - Print the board
	//    - Get user input
	//    - Move the player

	// Should the representation of the board be here or in the script code?

	LuaBridge bridge;	// Problem #1: There's a bunch of game specific code in the scriptbridge
						//             Create the luaBridge
	bridge.loadLUA(gameFile.c_str());	// Load the game

	BoardDriver driver(bridge);
	driver.loadBoard();

#if 0
	// KISS: start with a graph based board.
	// Need a representation here to put on the pieces.


	while(true) {
		
	}
#endif
}