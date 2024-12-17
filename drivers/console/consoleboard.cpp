#include "consoleboard.h"
#include "scriptbridge.h"

using namespace lurp;

void ConsoleBoardDriver(const std::string& gameDir)
{
	(void)gameDir;
	#if 0
	// Loop:
	//    - Print the board
	//    - Get user input
	//    - Move the player

	// Should the representation of the board be here or in the script code?

	LuaBridge bridge;	// Problem #1: There's a bunch of game specific code in the scriptbridge
						//             Create the luaBridge
	bridge.loadLUA("./game/walk/walk.lua");	// Load the game

	// KISS: start with a graph based board.
	// Need a representation here to put on the pieces.


	while(true) {
		
	}

	#endif
}