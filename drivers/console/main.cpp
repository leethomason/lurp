#include "scriptbridge.h"
#include "zonedriver.h"
#include "scriptdriver.h"
#include "test.h"

// Battle dev. fixme: remove.
#include "battle.h"	// fixme: in development. remove.
#include "scriptasset.h"

// Console driver includes
#include "dye.h"
#include "argh.h"
#include "crtdbg.h"
#include <fmt/core.h>

// C++
#include <stdio.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <array>
#include <filesystem>

using namespace lurp::swbattle;

static constexpr int N_RANGE = 4;
static constexpr char* RANGE[N_RANGE] = { "Short", "Medium", "Long", "Extreme" };
static constexpr int N_COVER = 5;
static constexpr char* COVER[N_COVER] = { "None", "Light", "Medium", "Heavy", "Full"};

struct Value
{
	// d		kChar	
	// d2		kCharInt
	// d32		kCharInt
	// 1		kInt
	// Sam		kString
	// all of the above
	// d32 2    option=2

	enum class Type {
		kNone,
		kChar,
		kCharInt,
		kInt,
		kString,
	};

	char cVal = 0;			
	int intVal = 0;
	std::string strVal;
	std::string rawStr;
	int option = INT_MIN;

	Type type = Type::kNone;

	bool intInRange(int end) const {
		return type == Type::kInt && intVal >= 0 && intVal < end;
	}
	bool charIntInRange(char c, int end) const {
		return type == Type::kCharInt && c == cVal && intVal >= 0 && intVal < end;
	}
	bool hasOption() const { return option != INT_MIN; }
};

static std::string ReadString()
{
	std::string r;

	while (true) {
		char c = (char) getchar();
		if (c == '\n') break;
		r += c;
	}
	return r;
}

static Value ParseValue(const std::string& full)
{
	Value v;
	v.rawStr = full;
	if (full.empty()) return v;

	std::vector<std::string> s = split(full, ' ');
	assert(!s.empty());
	std::string str = s[0];
	if (s.size() > 1) {
		v.option = std::stoi(s[1]);
	}

	if (str.size() == 1) {
		if (std::isdigit(str[0])) {
			v.type = Value::Type::kInt;
			v.intVal = std::stoi(str);
		}
		else if (std::isalpha(str[0])) {
			v.type = Value::Type::kChar;
			v.cVal = str[0];
		}
		return v;
	}
	if (str.size() > 1) {
		if (std::isalpha(str[0]) && std::isdigit(str[1])) {
			v.type = Value::Type::kCharInt;
			v.cVal = str[0];
			v.intVal = std::stoi(str.c_str() + 1);
			return v;
		}
	}
	if (std::isdigit(str[0])) {
		v.type = Value::Type::kInt;
		v.intVal = std::stoi(str);
		return v;
	}
	v.type = Value::Type::kString;
	v.strVal = str;
	return v;
}

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

static void PrintTurnOrder(const BattleSystem& system)
{
	const auto& turnOrder = system.turnOrder();
	const auto& combatants = system.combatants();
	fmt::print("\nOrder:\n");
	for(int i=0; i<turnOrder.size(); i++) {
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
		fmt::print("'{}' cover={}\n", r.name, COVER[(int)r.cover]);

		for(size_t cIndex = 0; cIndex < combatants.size(); cIndex++) {
			const Combatant& c = combatants[cIndex];
			if (c.dead() || c.region != rIndex) {
				continue;
			}
			else {
				fmt::print("    {}:'{}' ({}{}{}{}{}) toughness={} ({}) shaken={} wounds={} \n",
					cIndex,
					c.name,
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

	fmt::print("  toughness={} ({})\n",
		defender.toughness() + defender.armor.armor,
		defender.armor.armor);

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
	while(!system.queue.empty()) {
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

static void ConsoleBattleDriver()
{
	Random random(0xabc);
	random.rand();

	BattleSystem system(random);
	std::vector<Region> regions = {
		{ "Stalagmites", 0, Cover::kLightCover },
		{ "Shallow pool", 8, Cover::kNoCover },
		{ "Crumbling stone", 16, Cover::kMediumCover },
		{ "Wide steps up", 24, Cover::kNoCover}
	};
	system.setBattlefield("Cursed Cavern");
	for(const Region& r : regions)
		system.addRegion(r);

	Combatant player;
	Combatant skele0;
	Combatant skele1;

	MeleeWeapon longsword = { "longsword", Die(1, 8, 0), 8, true};
	MeleeWeapon shortsword = { "shortsword", Die(1, 6, 0), 6, false};
	RangedWeapon bow = { "bow", Die(1, 6, 0), 0, 6, 24 };				
	Armor chain = { "chain", 3, 8 };
	Armor leather = { "leather", 2, 6 };
	Armor hide = {"hide", 1, 4};

	// type, name, cost, range, effect
	Power arcaneBolt = { ModType::kBolt, "Fire Bolt", 1, 2 };
	Power getSwol = { ModType::kStrength, "Get Swol", 3, 0};
	Power darkmind = { ModType::kBoost, "Dark Mind", 3, 1, -1 };
	Power heal = { ModType::kHeal, "Heal", 3, 0 };
	Power frail = { ModType::kStrength, "Frail", 3, 1, -2 };
	Power shell = { ModType::kPowerCover, "Shield of Force", 2, 0, 2 };

	player.name = "Player";
	player.wild = true;

	player.agility = Die(1, 6, 0);
	player.spirit = Die(1, 6, 0);
	player.strength = Die(1, 8, 0);
	
	player.fighting = Die(1, 8, 0);
	player.shooting = Die(1, 6, 0);
	player.arcane = Die(1, 6, 0);

	player.meleeWeapon = longsword;
	player.rangedWeapon = bow;
	player.armor = chain;
	
	player.powers.push_back(getSwol);
	player.powers.push_back(heal);
	player.powers.push_back(arcaneBolt);

	skele0.name = "Skeleton Warrior";
	skele0.region = int(regions.size()) - 1;
	skele0.strength = Die(1, 8, 0);
	skele0.fighting = Die(1, 6, 0);
	skele0.shooting = Die(1, 4, 0);
	skele0.meleeWeapon = longsword;
	skele0.armor = hide;

	skele1.name = "Skeleton Archer";
	skele1.strength = Die(1, 6, 0);
	skele1.fighting = Die(1, 4, 0);
	skele1.shooting = Die(1, 6, 0);
	skele1.meleeWeapon = shortsword;
	skele1.rangedWeapon = bow;
	skele1.armor = hide;

	system.addCombatant(player);
	system.addCombatant(skele0);
	system.addCombatant(skele1);

	system.start();

	PrintTurnOrder(system);
	fmt::print("\n");

	while (!system.done()) {
		if (system.playerTurn()) {
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
				for(size_t i=0; i<pc.powers.size(); i++) {
					const Power& p = pc.powers[i];

					fmt::print("(p{} #) {} ({}%)\n", i, p.name, int(100.0 * system.powerChance(pc.index, p)));
				}
			}
			fmt::print("(d)one\n");
			fmt::print("> ");
			Value v = ParseValue(ReadString());
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

static void PrintNews(NewsQueue& queue)
{
	while (!queue.empty()) {
		NewsItem ni = queue.pop();
		if (ni.type == NewsType::kItemDelta) {
			assert(ni.item);
			int bias = ni.delta < 0 ? -1 : 1;
			fmt::print("{}You {} {} {}{}\n", 
				bias > 0 ? dye::green : dye::red,
				ni.verb(), ni.delta * bias, ni.item->name,
				dye::reset
			);
		}
		else if (ni.type == NewsType::kLocked || ni.type == NewsType::kUnlocked) {
			fmt::print("{}The {} was {}", dye::green, ni.noun(), ni.verb());
			if (ni.item)
				fmt::print(" by the {}", ni.item->name);
			fmt::print("{}\n", dye::reset);
		}
	}
}

static void ConsoleScriptDriver(ScriptAssets& assets, ScriptBridge& bridge, const ScriptEnv& env, MapData& mapData)
{
	ScriptDriver dd(assets, env, mapData, &bridge);

	while (!dd.done()) {
		while (dd.type() == ScriptType::kText) {
			TextLine line = dd.line();
			if (line.speaker.empty())
				fmt::print("{}\n", line.text);
			else
				fmt::print("{}: {}\n", line.speaker, line.text);
			dd.advance();
			PrintNews(mapData.newsQueue);
		}
		if (dd.done())
			break;

		if (dd.type() == ScriptType::kChoices) {
			const Choices& choices = dd.choices();
			int i = 0;
			for (const Choices::Choice& c : choices.choices) {
				fmt::print("{}: {}\n", i, c.text);
				i++;
			}
			Value v2 = ParseValue(ReadString());
			if (v2.intInRange((int)choices.choices.size()))
				dd.choose(v2.intVal);
			PrintNews(mapData.newsQueue);
		}
		else if (dd.type() == ScriptType::kBattle) {
			/*
			const Battle& battle = dd.battle();
			Random rand(clock());
			Battler hero = Battler::read(bridge, env.player, rand);
			Battler enemy = Battler::read(bridge, battle.enemy, rand);
			bool won = BattleConsoleDriver(hero, enemy, rand);
			if (won) {
				Battler::write(bridge, env.player, hero);
				ScriptRef ref = assets.get(battle.entityID);
				assets.battles[ref.index].done = true;
			}
			*/
			dd.advance();
		}
		else {
			assert(false);
		}
	}
}

static void PrintInventory(const Inventory& inv, bool label, const std::string& lead)
{
	if (label) fmt::print("Inventory:\n");
	static int COLS = 4;
	int nItem = 0;
	for (const auto& itemRef : inv.items()) {
		const Item& item = *itemRef.pItem;
		std::string t = fmt::format("{}: {}", item.name, itemRef.count);
		if (nItem % COLS == 0)
			fmt::print("{}", lead);
		fmt::print("{:<19}", t);
		if (nItem % COLS == COLS - 1)
			fmt::print("\n");
		++nItem;
	}
	if (nItem == 0)
		fmt::print("{}(empty)\n", lead);
	fmt::print("\n");
}

static void PrintContainers(ZoneDriver& driver, const ContainerVec& vec)
{
	int idx = 0;
	for (const auto container : vec) {
		if (idx == 0) fmt::print("Containers: \n");
		bool locked = driver.locked(*container);
		fmt::print("  c{}: {} {}\n", idx++, container->name, locked ? "(locked)" : "");
		if (!locked) {
			const Inventory& inv = driver.getInventory(*container);
			PrintInventory(inv, false, "    ");
		}
	}
}

static void PrintInteractions(const InteractionVec& vec, const ScriptAssets&)
{
	int idx = 0;
	for (const auto i : vec) {
		if (idx == 0) fmt::print("Here: \n");
		fmt::print("  i{}: {}\n", idx++, i->name);
	}
}

static void PrintEdges(const std::vector<DirEdge>& edges) {
	int idx = 0;
	fmt::print("Go:\n");
	for (const DirEdge& e : edges) {
		if (e.dir != Edge::Dir::kUnknown)
			fmt::print("  {}: {} {}\n", e.dirShort, e.name, e.locked ? " (locked)" : "");
		else
			fmt::print("  {}: {} {}\n", idx, e.name, e.locked ? " (locked)" : "");
		idx++;
	}

}

static bool ProcessMenu(const std::string& s, const std::string& path, ZoneDriver& zd)
{
	if (s == "/s" || s == "/c") {
		fmt::print("Saving...\n");
		std::ofstream stream;
		stream.open(path, std::ios::out);
		assert(stream.is_open());
		zd.save(stream);
	}
	if (s == "/l" || s == "/c") {
		fmt::print("Loading...\n");
		ScriptBridge loader;
		loader.loadLUA(path);
		zd.load(loader);
	}
	if (s == "/q") {
		return true;
	}
	return false;
}

static void PrintRoomDesc(const Zone& zone, const Room& room)
{
	fmt::print("{}{}{}\n", dye::white, room.name, dye::reset);
	fmt::print("{}{}{}\n", dye::reset,  zone.name, dye::reset);
	if (!room.desc.empty()) fmt::print("{}{}{}\n", dye::reset, room.desc, dye::reset);
	fmt::print("\n");
}

static int SelectEdge(const Value& v, const std::vector<DirEdge>& edges)
{
	if (v.intInRange((int)edges.size())) {
		return v.intVal;
	}
	for (size_t i = 0; i < edges.size(); i++) {
		if (toLower(v.rawStr) == toLower(edges[i].dirShort)) {
			return (int)i;
		}
	}
	return -1;
}

static void ConsoleZoneDriver(ScriptAssets& assets, ScriptBridge& bridge, EntityID zone, std::string saveFile)
{
	ZoneDriver driver(assets, &bridge, zone, "player");
	while (true) {
		ZoneDriver::Mode mode = driver.mode();

		if (mode == ZoneDriver::Mode::kText) {
			while (driver.mode() == ZoneDriver::Mode::kText) {
				// FIXME: support speaker mode
				TextLine tl = driver.text();
				fmt::print("{}\n", tl.text);
				driver.advance();
			}
			PrintNews(driver.mapData.newsQueue);
		}
		else if (mode == ZoneDriver::Mode::kChoices)
		{
			const Choices& choices = driver.choices();
			for (size_t i = 0; i < choices.choices.size(); i++) {
				fmt::print("{}: {}\n", i, choices.choices[i].text);
			}
			Value v2 = ParseValue(ReadString());
			if (v2.intInRange((int)choices.choices.size()))
				driver.choose(v2.intVal);
			PrintNews(driver.mapData.newsQueue);
		}
		else if (mode == ZoneDriver::Mode::kNavigation) {
			if (!driver.endGameMsg().empty())
				break;

			fmt::print("\n");
			PrintRoomDesc(driver.currentZone(), driver.currentRoom());
			const Actor& player = driver.getPlayer();
			const ContainerVec& containerVec = driver.getContainers();
			const InteractionVec& interactionVec = driver.getInteractions();
			std::vector<DirEdge> edges = driver.dirEdges();

			PrintInventory(assets.getInventory(player), true, "  ");
			PrintContainers(driver, containerVec);
			PrintInteractions(interactionVec, assets);
			PrintEdges(edges);
			fmt::print("Menu: (/s)ave (/l)oad (/c)ycle (/q)uit\n");

			fmt::print("> ");

			Value v = ParseValue(ReadString());
			if (ProcessMenu(v.rawStr, saveFile, driver)) {
				break;
			}
			else if (v.charIntInRange('c', (int)containerVec.size())) {
				const Container* c = driver.getContainer(containerVec[v.intVal]->entityID);
				
				ZoneDriver::TransferResult tr = driver.transferAll(*c, player);
				if (tr == ZoneDriver::TransferResult::kLocked)
					fmt::print("{}Container is locked.{}\n", dye::red, dye::reset);
			}
			else if (v.charIntInRange('i', (int)interactionVec.size())) {
				driver.startInteraction(interactionVec[v.intVal]);
			}
			else {
				int dirIdx = SelectEdge(v, edges);
				if (dirIdx >= 0) {
					if (driver.move(edges[dirIdx].dstRoom) == ZoneDriver::MoveResult::kLocked)
						fmt::print("{}That way is locked.{}\n", dye::red, dye::reset);
				}
			}
			if (driver.mode() == ZoneDriver::Mode::kNavigation) {
				PrintNews(driver.mapData.newsQueue);
			}
		}
		else {
			assert(false);
			break;
		}
	}
	if (!driver.endGameMsg().empty()) {
		fmt::print("{}{}{}\n", dye::white, driver.endGameMsg(), dye::reset);
	}
}

std::string FindSavePath(const std::string& scriptFile)
{
	std::ifstream stream;
	stream.open("game/saves/saves.md", std::ios::in);
	if (!stream.is_open()) {
		fmt::print("WARNING - no save path found\n");
		fmt::print("Save and load will be to the current directory.\n");
		return "save.lua";
	}
	stream.close();
	std::filesystem::path p = scriptFile;
	std::string result = std::string("game/saves/save-") + p.stem().string() + ".lua";
	fmt::print("Save path: {}\n", result);
	return result;
}

static void TestValueParsing()
{
	Value v;
	v = ParseValue("d");
	TEST(v.type == Value::Type::kChar);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 0);
	TEST(v.hasOption() == false);

	v = ParseValue("d2");
	TEST(v.type == Value::Type::kCharInt);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 2);
	TEST(v.hasOption() == false);

	v = ParseValue("d32");
	TEST(v.type == Value::Type::kCharInt);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 32);

	v = ParseValue("d32 2");
	TEST(v.type == Value::Type::kCharInt);
	TEST(v.cVal == 'd');
	TEST(v.intVal == 32);
	TEST(v.hasOption() == true);
	TEST(v.option == 2);

	v = ParseValue("1");
	TEST(v.type == Value::Type::kInt);
	TEST(v.intVal == 1);

	v = ParseValue("Sam");
	TEST(v.type == Value::Type::kString);
	TEST(v.strVal == "Sam");

	v = ParseValue("Sam 2");
	TEST(v.type == Value::Type::kString);
	TEST(v.strVal == "Sam");
	TEST(v.hasOption() == true);
	TEST(v.option == 2);
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

void RunConsoleTests()
{
	RUN_TEST(TestValueParsing());
	RUN_TEST(TestRollStr());
	RUN_TEST(TestModStr());
}


int main(int argc, const char* argv[])
{
	fmt::print("LuRP\n");
	if (argc == 1) {
		fmt::print("LuRP story engine. This is the console line runner.\n");
		fmt::print("Usage: lurp <path/to/lua/file> <starting-zone>\n");
		fmt::print("Optional:\n");
		fmt::print("  -b, --battle		Run the battle sim\n");
		fmt::print("  -t, --trace		Enable debug tracing\n");
		fmt::print("  -s, --debugSave   Save everything with warnings.\n");
	}
	int rc = 0;

#ifdef  _DEBUG
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	{
		argh::parser cmdl(argv);
		std::string scriptFile = cmdl[1];
		std::string startingZone = cmdl[2];
		bool trace = cmdl[{ "-t", "--trace" }];
		bool debugSave = cmdl[{ "-s", "--debugSave" }];
		bool battleSim = cmdl[{ "-b", "--battle" }];

		{
			RunTests();
			RunConsoleTests();
			rc = TestReturnCode();
		}
		Globals::trace = trace;
		Globals::debugSave = debugSave;

		std::string saveFile = FindSavePath(scriptFile);

		if (battleSim) {
			ConsoleBattleDriver();
		}
		else {
			// Run game.
			if (!scriptFile.empty() && !startingZone.empty()) {
				ScriptBridge bridge;
				ConstScriptAssets csassets = bridge.readCSA(scriptFile);
				ScriptAssets assets(csassets);

				ScriptRef ref = assets.get(argv[2]);
				if (ref.type == ScriptType::kScript) {
					ScriptEnv env;
					env.script = argv[2];
					env.player = "player";
					MapData mapData(&bridge, 567 + clock());
					ConsoleScriptDriver(assets, bridge, env, mapData);
				}
				else if (ref.type == ScriptType::kZone) {
					fmt::print("\n\n");
					ConsoleZoneDriver(assets, bridge, startingZone, saveFile);
				}
			}
		}
	}

#ifdef _DEBUG
	int knownNumLeak = 0;
	int knownLeakSize = 0;

	printf("Memory report:\n");
	_CrtMemCheckpoint(&s2);
	_CrtMemDifference(&s3, &s1, &s2);
	_CrtMemDumpStatistics(&s3);
	printf("Leak count=%lld size=%lld\n", s3.lCounts[1], s3.lSizes[1]);
	printf("High water=%lldk nAlloc=%lld\n", s3.lHighWaterCount / 1024, s3.lTotalCount);
	assert(s3.lCounts[1] <= knownNumLeak);
	assert(s3.lSizes[1] <= knownLeakSize);
#endif
	LogTests();
	if (rc == 0)
		fmt::print("LuRP tests run successfully.\n");
	else
		fmt::print("LuRP tests failed.\n");
	return rc;
}
