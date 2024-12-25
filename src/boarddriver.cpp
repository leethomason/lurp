#include "boarddriver.h"
#include "luabridge.h"

namespace lurp {

void BoardDriver::loadBoard()
{
	//bridge.callGlobalFunc("loadBoard");
	std::vector<Variant> args;
	std::vector<Variant> results;
	int nResults = bridge.callGlobalFunc("loadBoard", args);
	bridge.pop(nResults);
}

} // namespace lurp


