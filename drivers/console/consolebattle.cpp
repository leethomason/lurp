#include "consolebattle.h"
#include "battle.h"
#include "consoleutil.h"
#include "test.h"

#include <fmt/format.h>

using namespace lurp::swbattle;
using namespace lurp;

static constexpr int N_RANGE = 4;
static constexpr char* RANGE[N_RANGE] = { "Short", "Medium", "Long", "Extreme" };
static constexpr int N_COVER = 5;
static constexpr char* COVER[N_COVER] = { "None", "Light", "Medium", "Heavy", "Full" };

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

		r += fmt::format("{:+d} {}", m.delta, name);
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

static void PrintTurnOrder(const BattleSystem& system)
{
	const auto& turnOrder = system.turnOrder();
	const auto& combatants = system.combatants();
	fmt::print("\nOrder:\n");
	for (int i = 0; i < turnOrder.size(); i++) {
		int index = turnOrder[i];
		fmt::print("{}: {}\n", i, combatants[index].name);
	}
}

static void PrintCombatants(const BattleSystem& system)
{
	using SWB = BattleSystem;
	const auto& regions = system.regions();
	const auto& combatants = system.combatants();

	for (size_t rIndex = 0; rIndex < regions.size(); ++rIndex) {
		const Region& r = regions[rIndex];
		fmt::print("'{}' @ {} yards cover={}\n", r.name, r.yards, COVER[(int)r.cover]);

		for (size_t cIndex = 0; cIndex < combatants.size(); cIndex++) {
			const Combatant& c = combatants[cIndex];
			if (c.dead() || c.region != rIndex) {
				continue;
			}
			else {
				fmt::print("    {}:'{}' [f={} s={} a={}] ({}{}{}{}{}) toughness={} ({}) shaken={} wounds={} \n",
					cIndex,
					c.name,
					DieStr(c.fighting), DieStr(c.shooting), DieStr(c.arcane),
					c.meleeWeapon.name, c.hasRanged() ? ", " : "", c.rangedWeapon.name,
					c.hasArmor() ? " - " : "", c.armor.name,
					c.toughness() + c.armor.armor, c.armor.armor,
					c.shaken, c.wounds);
				if (c.activePowers.size() > 0) {
					fmt::print("      Powers: ");
					for (const auto& p : c.activePowers) {
						fmt::print("{} ", p.power->name);
					}
					fmt::print("\n");
				}
			}
		}
	}
}

static void PrintDamageReport(const Combatant& defender,
	bool melee,
	const DamageReport& damage,
	const std::vector<ModInfo>& damageMods)
{
	if (melee)
		fmt::print("  Hit! damage roll: ({}) + ({}) = {} ",
			RollStr(damage.damageRoll),
			RollStr(damage.strengthRoll),
			damage.damage);
	else
		fmt::print("  Hit! damage roll: {} ",
			RollStr(damage.damageRoll));

	if (!damageMods.empty())
		fmt::print("({})", ModStr(damageMods));
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
		const Action& action = system.queue.front();
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
				PrintDamageReport(defender, a.melee, a.damage, a.damageMods);
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

			if (!a.activated) {
				fmt::print("{} tried to use {} on {} but failed.\n", src.name, a.power->name, dst.name);
			}
			else {
				fmt::print("{} used {} on {}.\n", src.name, a.power->name, dst.name);
				if (a.raise)
					fmt::print("  Raise: {}\n", a.raise);
				if (a.power->doesDamage())
					PrintDamageReport(dst, false, a.damage, std::vector<ModInfo>());
			}
			break;
		}

		}
		system.queue.pop();
	}
}

static int AssignCombatant(int era, Combatant* c, int n, const BattleSpec& spec, Random& random)
{
	MeleeWeapon mw, mwPlus;
	RangedWeapon rw;
	Armor armor;
	Power power0, power1, power2;
	std::string names[3];

	if (era == 0) {
		mwPlus = { "longsword", Die(1, 8, 0), 8, true };
		mw = { "shortsword", Die(1, 6, 0), 6, false };
		rw = { "bow", Die(1, 6, 0), 0, 6, 24 };
		armor = { "chain", 3, 8 };

		// type, name, cost, range, effect
		power0 = { ModType::kBolt, "Fire Bolt", 1, 2 };
		power1 = { ModType::kHeal, "Heal", 3, 0 };
		power2 = { ModType::kPowerCover, "Shield of Force", 2, 0, 2 };

		names[0] = "Skeleton Warrior";
		names[1] = "Skeleton Archer";
		names[2] = "Skeleton Mage";
	}
	else if (era == 1) {
		mw = mwPlus = { "knife", Die(1, 4, 0), 4, false };
		rw = { "revolver", Die(2, 6, 1), 1, 4, 24 };

		power0 = { ModType::kBolt, "Arcane Bolt", 1, 2 };
		power1 = { ModType::kHeal, "Blessed Heal", 3, 0 };
		power2 = { ModType::kBolt, "Dark Mind", 3, 1, -1 };

		names[0] = "Brawler";
		names[1] = "Gunslinger";
		names[2] = "Magi";
	}
	else {
		mw = { "knife", Die(1, 4, 0), 4, false };
		mwPlus = { "plasma sword", Die(1, 8, 0), 6, true };
		rw = { "blaster", Die(2, 6, 0), 2, 4, 30 };
		armor = { "plasteel", 4, 10 };

		power0 = { ModType::kPowerCover, "Force Shield", 2, 0, 2 };
		power1 = { ModType::kBoost, "Focus", 3, 1, 1 };
		power2 = { ModType::kBoost, "Confuse", 3, 2, -1 };

		names[0] = "Knight";
		names[1] = "Marine";
		names[2] = "Mystic";
	}

	int index = 0;
	for (int i = 0; i < spec.fighters; i++) {
		if (index >= n) continue;

		c[index].name = names[0];
		c[index].autoLevel(spec.level, 4, 1, 0, random.rand());
		c[index].meleeWeapon = mwPlus;
		c[index].armor = armor;
		c[index].powers.push_back(power0);
		c[index].powers.push_back(power1);
		index++;
	}
	for (int i = 0; i < spec.shooters; i++) {
		if (index >= n) continue;

		c[index].name = names[1];
		c[index].autoLevel(spec.level, 1, 4, 0, random.rand());
		c[index].meleeWeapon = mw;
		c[index].rangedWeapon = rw;
		c[index].armor = armor;
		c[index].powers.push_back(power0);
		index++;
	}
	for (int i = 0; i < spec.arcanes; i++) {
		if (index >= n) continue;

		c[index].name = names[2];
		c[index].autoLevel(spec.level, 1, 1, 6, random.rand());
		c[index].meleeWeapon = mw;
		c[index].armor = armor;
		c[index].powers.push_back(power0);
		c[index].powers.push_back(power1);
		c[index].powers.push_back(power2);
		index++;
	}
	return index;
}

void ConsoleBattleDriver(int era, const BattleSpec& playerBS, const BattleSpec& enemyBS, uint32_t seed)
{
	lurp::Random random(seed);
	BattleSystem system(random);

	static constexpr int kMaxEnemy = 10;
	Combatant player;
	Combatant enemy[kMaxEnemy];

	AssignCombatant(era, &player, 1, playerBS, random);
	player.name = "Player";
	player.wild = true;
	int nEnemy = AssignCombatant(era, enemy, kMaxEnemy, enemyBS, random);

	if (era == 0) {
		system.setBattlefield("Cursed Cavern");
		system.addRegion({ "Stalagmites", 0, Cover::kLightCover });
		system.addRegion({ "Shallow pool", 8, Cover::kNoCover });
		system.addRegion({ "Crumbling stone", 16, Cover::kMediumCover });
		system.addRegion({ "Wide steps up", 24, Cover::kNoCover });
	}
	else if (era == 1) {
		system.setBattlefield("Wild Weird West Bar");
		system.addRegion({ "Upturned Tables", 0, Cover::kMediumCover });
		system.addRegion({ "Open Floor", 5, Cover::kNoCover });
		system.addRegion({ "Bar", 10, Cover::kHeavyCover });
	}
	else {
		system.setBattlefield("Space Dock");
		system.addRegion({ "Loading Ramp", 0, Cover::kLightCover });
		system.addRegion({ "Catwalks", 10, Cover::kNoCover });
		system.addRegion({ "Shipping Crates", 20, Cover::kMediumCover });
	}

	system.addCombatant(player);
	for (int i = 0; i < nEnemy; i++) {
		system.addCombatant(enemy[i]);
	}
	system.start();

	PrintTurnOrder(system);
	fmt::print("\n");

	while (!system.done()) {
		if (system.playerTurn()) {
			PrintActions(system);
			PrintCombatants(system);
			fmt::print("\n");
			const Combatant& pc = system.combatants()[0];

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
					const Power& p = pc.powers[i];

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
			else if (v.type == Value::Type::kCharInt && v.hasOption() && within(v.option, 0, (int)pc.powers.size()))
				rc = system.power(0, v.intVal, v.option);
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
}

BattleSpec BattleSpec::Parse(const std::string& s)
{
	BattleSpec bs;
	if (s.size() > 0) bs.level = lurp::clamp(ctoi(s[0]), 1, 10);
	if (s.size() > 1) bs.fighters = lurp::clamp(ctoi(s[1]), 0, 10);
	if (s.size() > 2) bs.shooters = lurp::clamp(ctoi(s[2]), 0, 10);
	if (s.size() > 3) bs.arcanes = lurp::clamp(ctoi(s[3]), 0, 10);
	return bs;
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
	Power superBoost = { ModType::kBoost, "Super Boost", 3, 1, 2 };
	Power okayBoost = { ModType::kBoost, "Okay Boost", 2, 1, 1 };
	Power fuzzyMind = { ModType::kBoost, "Fuzzy Mind", 2, 1, -1 };
	Power iceWall = { ModType::kPowerCover, "Frozen", 1, 0, 1 };

	ActivePower apSuperBoost = { 1, &superBoost };
	ActivePower apOkayBoost = { 1, &okayBoost };
	ActivePower apFuzzyMind = { 1, &fuzzyMind };
	ActivePower apIceWall = { 1, &iceWall };

	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost, apIceWall };
		int result = BattleSystem::applyMods(ModType::kBoost, active, mods);

		TEST(result == 1);
		TEST(mods.size() == 1);
		TEST(ModStr(mods) == "+1 Okay Boost");
	}
	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost, apIceWall, apSuperBoost };
		int result = BattleSystem::applyMods(ModType::kBoost, active, mods);

		TEST(result == 2);
		TEST(mods.size() == 1);
		TEST(ModStr(mods) == "+2 Super Boost");
	}
	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost, apIceWall, apSuperBoost, apFuzzyMind };
		int result = BattleSystem::applyMods(ModType::kBoost, active, mods);

		TEST(result == 1);
		TEST(mods.size() == 2);
		TEST(ModStr(mods) == "+2 Super Boost, -1 Fuzzy Mind");
	}
	{
		std::vector<ModInfo> mods;
		std::vector<ActivePower> active = { apOkayBoost, apIceWall, apSuperBoost, apFuzzyMind };
		int result = BattleSystem::applyMods(ModType::kPowerCover, active, mods);

		TEST(result == 1);
		TEST(mods.size() == 1);
		TEST(ModStr(mods) == "+1 Frozen");
	}
}

void RunConsoleBattleTests()
{
	RUN_TEST(TestRollStr());
	RUN_TEST(TestModStr());

}
