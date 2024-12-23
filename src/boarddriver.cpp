#include "boarddriver.h"
#include "luabridge.h"

namespace lurp {

void BoardDriver::loadBoard()
{
	//bridge.callGlobalFunc("loadBoard");
	std::vector<Variant> args;
	std::vector<Variant> results;
	bridge.callGlobalFunc("loadBoard", args, results);
}

} // namespace lurp


