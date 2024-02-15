#pragma once

#include <string>

struct BattleSpec {
	int level = 5;
	int fighters = 1;
	int shooters = 0;
	int arcanes = 0;

	static BattleSpec Parse(const std::string& str);
};

void ConsoleBattleSim(int era, const BattleSpec& playerBS, const BattleSpec& enemyBS, uint32_t seed);
bool ConsoleBattleDriver();
void RunConsoleBattleTests();
