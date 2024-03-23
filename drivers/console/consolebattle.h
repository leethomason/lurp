#pragma once

#include <string>

#include "defs.h"

namespace lurp {
struct ScriptAssets;
struct Battle;
class Random;
class VarBinder;
}

bool ConsoleBattleDriver(const lurp::ScriptAssets& assets, const lurp::VarBinder& binder, const lurp::Battle& battle, lurp::EntityID player, lurp::Random& r);

void RunConsoleBattleTests();
void BattleOutputTests();

