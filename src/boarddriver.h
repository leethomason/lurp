#pragma once

namespace lurp {

class LuaBridge;

class BoardDriver {
public:
	BoardDriver(LuaBridge& bridge) : bridge(bridge) {}

	void loadBoard();

private:
	LuaBridge& bridge;
};

} // namespace lurp