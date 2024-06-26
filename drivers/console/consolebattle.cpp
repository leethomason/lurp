#include "consolebattle.h"
#include "battle.h"
#include "consoleutil.h"
#include "test.h"

#include <fmt/format.h>
#include <ionic/ionic.h>

using namespace lurp::swbattle;
using namespace lurp;

static constexpr int N_COVER = 5;
static const char* COVER[N_COVER] = { "None", "Light", "Medium", "Heavy", "Full" };

static std::string RollStr(const Roll& roll)
{
	// 6
	// 5 + 3 = 8
	// 6x2 + 5 = 17
	// (1, 2) = 2
	// (6x2 + 5 + 1 = 18, 2 + 1 = 3) = 18

	std::string s;
	if (roll.hasWild()) s += "(";

	for (int i = 0; i < (roll.hasWild() ? 2 : 1); i++) {
		if (roll.nAce(i) == 0 && roll.die.b == 0) {
			s += fmt::format("{}", roll.total[i]);
		}
		else {
			if (roll.nAce(i))
				s += fmt::format("[{}x{} + {}]", roll.die.d, roll.nAce(i), roll.finalRoll(i));
			else
				s += fmt::format("{}", roll.finalRoll(i));

			if (roll.die.b)
				s += fmt::format(" + {}", roll.die.b);

			s += fmt::format(" = {}", roll.total[i]);
		}
		if (i == 0 && roll.hasWild())
			s += fmt::format(", ");
	}
	if (roll.hasWild()) {
		s += ")";
		s += fmt::format(" = {}", roll.value());
	}
	return s;
}

static std::string ModStr(const std::vector<ModInfo>& mods)
{
	// Roll: (6x2 + 5 + 4 = 17, 2 + 4 = 6) = 17, (+2 cover, +1 Sorta Buf, +1 Swol)
	std::string r;
	bool first = true;
	for (const ModInfo& m : mods) {
		if (!first)
			r += ", ";
		first = false;

		std::string name = ModTypeName(m.type);
		if (m.power)
			name = m.power->name;

		r += fmt::format("{:+d} {}", m.delta
			, name);
	}
	return r;
}

static std::string DieStr(const Die& die)
{
	std::string r = fmt::format("{}d{}", die.n, die.d);
	if (die.b)
		r += fmt::format("{:+}", die.b);
	return r;
}

static void PrintTurnOrder(const std::vector<SWCombatant>& combatants, const std::vector<int>& turnOrder)
{
	ionic::TableOptions options;
	options.innerHDivider = false;
	ionic::Table table(options);

	fmt::print("\nOrder:\n");
	for (size_t i = 0; i < turnOrder.size(); i++) {
		table.addRow({ std::to_string(i), combatants[turnOrder[i]].name });
	}
	table.print();
}

static std::string EffectList(const std::vector<ActivePower> aps)
{
	std::string s;
	for(const ActivePower& ap : aps) {
		if (!s.empty())
			s += ", ";
		s += fmt::format("{} ({})", ap.power->name, ap.caster);
	}
	return s;
}

static void PrintCombatantsInRegion(int region, const std::vector<SWCombatant>& combatants)
{
	ionic::TableOptions options;
	options.outerBorder = false;
	options.innerHDivider = false;
	options.indent = 4;

	ionic::Table table(options);
	table.addRow({ "", "Name", "Fight", "Shoot", "Arcane", "Melee", "Range", "Armor", "Tough", "Shaken", "Wnds", "Effects"});

	int nRow = 0;
	for (size_t i = 0; i < combatants.size(); i++) {
		const SWCombatant& c = combatants[i];
		if (c.dead()) continue;
		if (c.region != region) continue;
		nRow++;

		std::string melee, range;
		if (c.hasMelee())
			melee = fmt::format("{} ({})", c.meleeWeapon.name, DieStr(c.meleeWeapon.damageDie));
		if (c.hasRanged())
			range = fmt::format("{} ({})", c.rangedWeapon.name, DieStr(c.rangedWeapon.damageDie));

		table.addRow({
			std::to_string(i),
			c.name,
			DieStr(c.fighting),
			DieStr(c.shooting),
			DieStr(c.arcane),
			melee,
			range,
			c.hasArmor() ? c.armor.name : "",
			fmt::format("{} ({})", c.toughness() + c.armor.armor, c.armor.armor),
			std::to_string(c.shaken),
			std::to_string(c.wounds),
			EffectList(c.activePowers)
			});

		ionic::Color color = (c.team == 0) ? ionic::Color::green : ionic::Color::brightRed;
		table.setRow(nRow, { color }, {});
	}
	if (nRow)
		table.print();
}

static void PrintRegion(const Region& r, bool first)
{
	ionic::TableOptions options;
	options.tableColor = options.textColor = ionic::Color::cyan;
	options.outerBorder = false;
	ionic::Table table(options);

	table.setColumnFormat({ 
		{ionic::ColType::fixed, 15},	// name
		{ionic::ColType::fixed, 5},		// yards
		{ionic::ColType::fixed, 10}		// cover
		});
	if (first)
		table.addRow({ "Name", "Yards", "Cover" });
	table.addRow({ r.name, std::to_string(r.yards), COVER[(int)r.cover] });
	table.print();
}

static void PrintCombatants(const std::vector<SWCombatant>& combatants, const std::vector<Region>& regions)
{
	for (size_t rIndex = 0; rIndex < regions.size(); ++rIndex) {
		const Region& r = regions[rIndex];
		PrintRegion(r, rIndex == 0);
		PrintCombatantsInRegion(int(rIndex), combatants);
	}
}

static void PrintDamageReport(const SWCombatant& defender,
	bool melee,
	const DamageReport& damage)
{
	if (melee)
		fmt::print("  Hit! damage roll ({}): ({}) + ({}) = {} ",
			damage.damageRoll.die.toString(),
			RollStr(damage.damageRoll),
			RollStr(damage.strengthRoll),
			damage.damage);
	else
		fmt::print("  Hit! damage roll ({}): {} ",
			damage.damageRoll.die.toString(),
			RollStr(damage.damageRoll));

	fmt::print("\n");

	fmt::print("  toughness={} ({})",
		defender.toughness() + defender.armor.armor,
		defender.armor.armor);

	if (damage.ap) {
		fmt::print(" -> toughness={} ({})",
			defender.toughness() + defender.armor.armor - damage.ap,
			defender.armor.armor - damage.ap);
	}
	fmt::print("\n");

	if (damage.defenderDead)
		fmt::print("  Defender is killed.\n");
	else if (damage.defenderWounds)
		fmt::print("  Defender takes {} wounds.\n", damage.defenderWounds);
	else if (damage.defenderShaken)
		fmt::print("  Defender is shaken.\n");
	else
		fmt::print("  ...but defender is unharmed.\n");
}

static void PrintActions(BattleSystem& system)
{
	while (!system.queue.empty()) {
		Action action = system.queue.pop();
		switch (action.type) {
		case Action::Type::kMove: {
			const MoveAction& a = std::get<MoveAction>(action.data);
			fmt::print("{} moves from '{}' to '{}'.\n",
				system.combatants()[a.combatant].name,
				system.regions()[a.from].name,
				system.regions()[a.to].name);
			break;
		}

		case Action::Type::kAttack: {
			const AttackAction& a = std::get<AttackAction>(action.data);
			const auto& attacker = system.combatants()[a.attacker];
			const auto& defender = system.combatants()[a.defender];

			fmt::print("{} attacks {} with {}.\n",
				attacker.name,
				defender.name,
				a.melee ? attacker.meleeWeapon.name : attacker.rangedWeapon.name);

			fmt::print("  {} attack roll: {} ", (a.melee ? "Melee" : "Ranged"), RollStr(a.attackRoll));
			if (!a.mods.empty())
				fmt::print("({})", ModStr(a.mods));
			fmt::print("\n");

			if (a.melee)
				fmt::print("  Defender parry: {}\n", defender.parry());
			else
				fmt::print("  TN = 4\n");

			if (a.success)
				PrintDamageReport(defender, a.melee, a.damageReport);
			else
				fmt::print("  Miss\n");
			break;
		}
		case Action::Type::kRecover: {
			const RecoverAction& a = std::get<RecoverAction>(action.data);
			if (a.shaken)
				fmt::print("{} is still shaken.\n", system.combatants()[a.combatant].name);
			else
				fmt::print("{} recovers from being shaken.\n", system.combatants()[a.combatant].name);
			break;
		}

		case Action::Type::kPower: {
			const PowerAction& a = std::get<PowerAction>(action.data);
			const auto& src = system.combatants()[a.src];
			const auto& dst = system.combatants()[a.target];

			fmt::print("{} uses {} ({}) on {}.\n", src.name, a.power->name, a.roll.die.toString(), dst.name);
			fmt::print("  Power roll: {} on tn of {} ", RollStr(a.roll), a.tn);
			if (!a.mods.empty())
				fmt::print("({})", ModStr(a.mods));
			fmt::print("\n");

			if (!a.activated) {
				fmt::print("  Power failed to activate.\n");
			}
			else {
				fmt::print("  {} succeeded.\n", a.power->name);
				if (a.raise)
					fmt::print("  Raise: {}\n", a.raise);
				if (a.power->doesDamage())
					PrintDamageReport(dst, false, a.damageReport);
			}
			break;
		}

		}
	}
}

void Pause()
{
	fmt::print("(enter) > ");
	ReadString();
}

bool ConsoleBattleDriver(const ScriptAssets& assets, const VarBinder& binder, const lurp::Battle& battle, EntityID player, Random& random)
{
	BattleSystem system(assets, binder, battle, player, random);
	system.start();

	PrintTurnOrder(system.combatants(), system.turnOrder());
	fmt::print("\n");

	while (!system.done()) {
		if (system.playerTurn()) {
			PrintActions(system);
			Pause();
			PrintCombatants(system.combatants(), system.regions());
			fmt::print("\n");
			const SWCombatant& pc = system.combatants()[0];

			fmt::print("Options:\n");
			if (system.checkMove(0, 1) == BattleSystem::ActionResult::kSuccess)
				fmt::print("(f)orward\n");
			if (system.checkMove(0, -1) == BattleSystem::ActionResult::kSuccess)
				fmt::print("(b)ack\n");

			if (pc.canAttack()) {
				std::string melee = pc.hasMelee() ? pc.meleeWeapon.name : "unarmed";
				if (pc.hasRanged())
					fmt::print("(a #) attack with {} ({}%) or {} ({}%)\n",
						melee,
						int(pc.baseMelee() * 100.0),
						pc.rangedWeapon.name,
						int(pc.baseRanged() * 100.0));
				else
					fmt::print("(a #) attack with {} ({}%)\n",
						melee,
						int(pc.baseMelee() * 100.0));
			}
			if (pc.canPowers()) {
				for (size_t i = 0; i < pc.powers.size(); i++) {
					const SWPower& p = pc.powers[i];

					fmt::print("(p{} #) {} ({}%)\n", i, p.name, int(100.0 * system.powerChance(pc.index, p)));
				}
			}
			fmt::print("(d)one\n");
			fmt::print("> ");
			Value v = Value::ParseValue(ReadString());
			BattleSystem::ActionResult rc = BattleSystem::ActionResult::kSuccess;

			if (v.rawStr == "f")
				rc = system.move(0, 1);
			else if (v.rawStr == "b")
				rc = system.move(0, -1);
			else if (v.type == Value::Type::kChar && v.hasOption() && within(v.option, 1, (int)system.combatants().size()))
				rc = system.attack(0, v.option);
			else if (v.type == Value::Type::kCharInt && v.hasOption() && within(v.intVal, 0, (int)pc.powers.size()))
				rc = system.power(0, v.option, v.intVal);
			else if (v.rawStr == "d")
				system.advance();

			if (rc == BattleSystem::ActionResult::kInvalid)
				fmt::print("Invalid action.\n");
			else if (rc == BattleSystem::ActionResult::kOutOfRange)
				fmt::print("Out of range.\n");
			else if (rc == BattleSystem::ActionResult::kNoAction)
				fmt::print("No action.\n");

			PrintActions(system);
		}
		else {
			system.doEnemyActions();
			PrintActions(system);
			system.advance();
		}
	}
	return system.victory();
}

static void TestRollStr()
{
	constexpr int noWild = std::numeric_limits<int>::min();
	TEST(RollStr({ Die(1, 8, 0), {6, noWild} }) == "6");
	TEST(RollStr({ Die(1, 6, 3), {8, noWild} }) == "5 + 3 = 8");
	TEST(RollStr({ Die(1, 6, 0), {17, noWild} }) == "[6x2 + 5] = 17");
	TEST(RollStr({ Die(1, 8, 0), {1, 2} }) == "(1, 2) = 2");
	TEST(RollStr({ Die(1, 6, 1), {18, 3} }) == "([6x2 + 5] + 1 = 18, 2 + 1 = 3) = 18");

	// (6x1 + 4 + -2 = 8, 5 + -2 = 3) = 8 (-2 Range)
	// (6+4 + -2 = 8, 5 + -2 = 3) = 8 (-2 Range)
	// ([6x1 + 4] + -2 = 8, 5 + -2 = 3 = 8) (-2 Range)
	TEST(RollStr({ Die(1, 6, -2), {8, 3} }) == "([6x1 + 4] + -2 = 8, 5 + -2 = 3) = 8");
}

static void TestModStr()
{
	SWPower superBoost = { ModType::kCombatBuff, "Super Boost", 3, 1, 2 };
	SWPower okayBoost = { ModType::kCombatBuff, "Okay Boost", 2, 1, 1 };
	SWPower fuzzyMind = { ModType::kRangeBuff, "Fuzzy Mind", 2, 1, -1 };

	ActivePower apSuperBoost = { 1, &superBoost };
	ActivePower apOkayBoost = { 1, &okayBoost };
	ActivePower apFuzzyMind = { 1, &fuzzyMind };

	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost };
		int result = BattleSystem::applyMods(uint32_t(ModType::kCombatBuff), active, mods);

		TEST(result == 2);
		TEST(mods.size() == 1);
		TEST(ModStr(mods) == "+2 Okay Boost");
	}
	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost, apSuperBoost };
		int result = BattleSystem::applyMods(uint32_t(ModType::kCombatBuff), active, mods);

		TEST(result == 6);
		TEST(mods.size() == 2);
		TEST(ModStr(mods) == "+2 Okay Boost, +4 Super Boost");
	}
	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost, apSuperBoost, apFuzzyMind };
		int result = BattleSystem::applyMods(uint32_t(ModType::kCombatBuff) | uint32_t(ModType::kRangeBuff), active, mods);

		TEST(result == 4);
		TEST(mods.size() == 3);
		TEST(ModStr(mods) == "+2 Okay Boost, +4 Super Boost, -2 Fuzzy Mind");
	}
}

void RunConsoleBattleTests()
{
	RUN_TEST(TestRollStr());
	RUN_TEST(TestModStr());
}

void BattleOutputTests()
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

	SWCombatant a = { "PLAYER", "Player" };
	SWCombatant b = { "ENEMY B", "Enemy B" };
	SWCombatant c = { "ENEMY C", "Enemy C" };

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

	SWPower superBoost = { ModType::kCombatBuff, "Super Boost", 3, 1, 2 };
	ActivePower apSuperBoost = { 1, &superBoost };
	SWPower fuzzyMind = { ModType::kRangeBuff, "Fuzzy Mind", 2, 1, -1 };
	ActivePower apFuzzyMind = { 0, &fuzzyMind };

	a.region = 0;
	a.meleeWeapon = { "sword", Die(1, 8, 0) };
	a.armor = { "chain", 3 };

	b.region = 2;
	b.rangedWeapon = { "bow", Die(1, 6, 0), 0, 24 };
	b.team = 1;
	b.activePowers.push_back(apSuperBoost);
	b.activePowers.push_back(apFuzzyMind);

	c.region = 2;
	c.meleeWeapon = { "knife", Die(1, 4, 0) };
	c.team = 1;

	Region r0 = { "Region 1", 0, Cover::kNoCover };
	Region r1 = { "Region 2", 10, Cover::kLightCover };
	Region r2 = { "Region 3", 20, Cover::kFullCover };

	std::vector<SWCombatant> combatants = { a, b, c };
	std::vector<int> order = { 2, 1, 0 };
	std::vector<Region> regions = { r0, r1, r2 };

	PrintTurnOrder(combatants, order);
	PrintCombatants(combatants, regions);
}

