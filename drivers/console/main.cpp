#include "scriptbridge.h"
#include "zonedriver.h"
#include "scriptdriver.h"
#include "test.h"
#include "consoleutil.h"

#include "consolebattle.h"
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

using namespace lurp;

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
			Value v2 = Value::ParseValue(ReadString());
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
			Value v2 = Value::ParseValue(ReadString());
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

			Value v = Value::ParseValue(ReadString());
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

void RunConsoleTests()
{
	RUN_TEST(Value::Test());
	RunConsoleBattleTests();
}


int main(int argc, const char* argv[])
{
	fmt::print("LuRP\n");
	if (argc == 1) {
		fmt::print("LuRP story engine. This is the console line runner.\n");
		fmt::print("Usage: lurp <path/to/lua/file> <starting-zone>\n");
		fmt::print("Optional:\n");
		fmt::print("  -t, --trace		Enable debug tracing\n");
		fmt::print("  -s, --debugSave   Save everything with warnings.\n");
		fmt::print("  -b, --battle		Run the battle sim\n");
		fmt::print("  -e, --enemies		<level><fight><shoot><arcane>\n");
		fmt::print("  -p, --player      <level><fight><shoot><arcane>\n");
		fmt::print("  -r, --era         0: ancient, 1: west, 2: future\n");
		fmt::print("  --seed            Random number seed\n");
	}
	int rc = 0;

#ifdef  _DEBUG
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	{
		argh::parser cmdl;
		cmdl.add_params({ "-e", "--enemies", "-p", "--player", "-r", "--era", "--seed" });
		cmdl.parse(argc, argv);

		std::string scriptFile = cmdl[1];
		std::string startingZone = cmdl[2];
		bool trace = cmdl[{ "-t", "--trace" }];
		bool debugSave = cmdl[{ "-s", "--debugSave" }];
		bool battleSim = cmdl[{ "-b", "--battle" }];

		BattleSpec enemyBS;
		BattleSpec playerBS;

		std::string v;
		cmdl({ "-e", "--enemies" }) >> v;
		enemyBS = BattleSpec::Parse(v);
		cmdl({ "-p", "--player" }) >> v;
		playerBS = BattleSpec::Parse(v);

		int era = 0;
		cmdl({ "-r", "--era" }, era) >> era;

		uint32_t seed = uint32_t(time(0));
		cmdl({ "--seed" }, seed) >> seed;

		{
			RunTests();
			RunConsoleTests();
			rc = TestReturnCode();
			LogTests();
			if (rc == 0)
				fmt::print("LuRP tests run successfully.\n");
			else
				fmt::print("LuRP tests failed.\n");
		}
		Globals::trace = trace;
		Globals::debugSave = debugSave;

		std::string saveFile = FindSavePath(scriptFile);

		if (battleSim) {
			ConsoleBattleSim(era, playerBS, enemyBS, seed);
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
	return rc;
}
