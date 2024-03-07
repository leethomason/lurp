#pragma once

#include <string>
#include "scriptasset.h"	// FIXME: lurp.h?

namespace lurp {
class Varbinder;
}

struct BattleSpec {
	int level = 5;
	int fighters = 1;
	int shooters = 0;
	int arcanes = 0;

	static BattleSpec Parse(const std::string& str);
};

void ConsoleBattleSim(int era, const BattleSpec& playerBS, const BattleSpec& enemyBS, uint32_t seed);
bool ConsoleBattleDriver(const lurp::ScriptAssets& assets, const lurp::VarBinder& binder, const lurp::Battle& battle, lurp::EntityID player, lurp::Random& r);

void RunConsoleBattleTests();
void BattleOutputTests();

