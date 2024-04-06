#include "scriptbridge.h"
#include "zonedriver.h"
#include "scriptdriver.h"
#include "test.h"
#include "consoleutil.h"

#include "consolebattle.h"
#include "scriptasset.h"
#include "varbinder.h"

#include "../platform.h"

// Console driver includes
#include "argh.h"
#ifdef _WIN32
#include "crtdbg.h"
#endif
#include <fmt/core.h>
#include <ionic/ionic.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>

// C++
#include <stdio.h>
#include <assert.h>

#include <string>
#include <vector>
#include <algorithm>
#include <array>
#include <filesystem>

using namespace lurp;

static constexpr ionic::Color configTextColor = ionic::Color::yellow;
static constexpr ionic::Color configChoiceColor = ionic::Color::cyan;

static void PrintTextLine(const std::string& text, ionic::Color color)
{
	ionic::TableOptions options;
	options.outerBorder = false;
	options.textColor = color;

	ionic::Table table(options);
	table.addRow({ ionic::Table::normalizeMD(text, 2) });
	table.print();
}

static void PrintText(std::string speaker, const std::string& text)
{
	ionic::TableOptions options;
	options.outerBorder = false;
	options.innerHDivider = false;
	options.innerVDivider = false;
	options.textColor = configTextColor;
	ionic::Table table(options);

	static const int MDNL = 2;

	if (speaker.empty()) {
		table.addRow({ ionic::Table::normalizeMD(text, MDNL) });
	}
	else {
		table.setColumnFormat({ {ionic::ColType::fixed, 12}, {ionic::ColType::flex} });
		table.addRow({ speaker, ionic::Table::normalizeMD(text, MDNL) });
	}
	table.print();
}

static void PrintChoices(const Choices& choices)
{
	ionic::TableOptions options;
	options.outerBorder = false;
	options.innerHDivider = false;
	options.textColor = configChoiceColor;
	options.tableColor = configChoiceColor;
	ionic::Table table(options);

	int i = 0;
	for (const Choices::Choice& c : choices.choices) {
		table.addRow({ std::to_string(i), ionic::Table::normalizeMD(c.text, 2) });
		i++;
	}
	table.print();
}

static void PrintNews(NewsQueue& queue)
{
	while (!queue.empty()) {
		NewsItem ni = queue.pop();
		if (ni.type == NewsType::kItemDelta) {
			assert(ni.item);
			int bias = ni.delta < 0 ? -1 : 1;
			ionic::Color color = ni.delta < 0 ? ionic::Color::red : ionic::Color::green;
			PrintTextLine(
				fmt::format("You {} {} {}", ni.verb(), ni.delta * bias, ni.item->name),
				color
			);
		}
		else if (ni.type == NewsType::kLocked || ni.type == NewsType::kUnlocked) {
			std::string s = fmt::format("The {} was {}",ni.noun(), ni.verb());
			if (ni.item)
				s += fmt::format(" by the {}", ni.item->name);
			PrintTextLine(s, ionic::Color::green);
		}
	}
}

static void PrintInventory(const Inventory& inv)
{
	if (inv.emtpy()) {
		fmt::print("Inventory empty.\n");
		return;
	}

	ionic::TableOptions options;
	options.outerBorder = false;

	std::vector<std::string> vstr;

	for (const auto& itemRef : inv.items()) {
		vstr.push_back(fmt::format("{}: {}", itemRef.pItem->name, itemRef.count));
	}

	ionic::Table table(options);
	table.addRow(vstr);
	table.print();
}

static void PrintContainers(ZoneDriver& driver, const ContainerVec& vec)
{
	if (vec.empty())
		return;

	ionic::TableOptions options;
	options.outerBorder = false;
	options.tableColor = configChoiceColor;
	options.textColor = configChoiceColor;
	ionic::Table table(options);

	int idx = 0;
	for (const auto container : vec) {
		if (idx == 0) fmt::print("Containers: \n");
		bool locked = driver.locked(*container);
		std::string istr;
		if (!locked) {
			const Inventory& inv = driver.getInventory(*container);
			for (const auto& itemRef : inv.items()) {
				if (!istr.empty())
					istr += " | ";
					istr += fmt::format("{}: {}", itemRef.pItem->name, itemRef.count);
			}
		}
		std::string option = fmt::format("c{}", idx);
		table.addRow({ option, container->name, locked ? "(locked)" : "", istr });
		idx++;
	}
	table.print();
}

static void PrintInteractions(const InteractionVec& vec, const ScriptAssets&)
{
	if (vec.empty())
		return;

	ionic::TableOptions options;
	options.outerBorder = false;
	options.tableColor = configChoiceColor;
	options.textColor = configChoiceColor;
	ionic::Table table(options);

	int i = 0;
	for (const auto iact : vec) {
		std::string s = fmt::format("i{}", i);
		table.addRow({ s, iact->name });
		i++;
	}
	table.print();
}

static void PrintEdges(const std::vector<DirEdge>& edges) {
	int idx = 0;
	fmt::print("Go:\n");

	ionic::TableOptions options;
	options.outerBorder = false;
	options.innerHDivider = false;
	options.textColor = configChoiceColor;
	options.tableColor = configChoiceColor;

	ionic::Table table(options);

	for (const DirEdge& e : edges) {
		if (e.dir != Edge::Dir::kUnknown)
			table.addRow({ e.dirShort, e.name, e.locked ? " (locked)" : "" });
		else
			table.addRow({ std::to_string(idx), e.name, e.locked ? " (locked)" : "" });
		idx++;
	}
	std::cout << table << std::endl;
}

static bool ProcessMenu(const std::string& s, const std::string& dir, ZoneDriver& zd)
{
	std::string path = SavePath(dir, "save");

	if (s == "/s" || s == "/c") {
		fmt::print("Saving to '{}'...\n", path);
		std::ofstream stream = OpenSaveStream(path);
		zd.save(stream);
	}
	if (s == "/l" || s == "/c") {
		fmt::print("Loading from '{}'...\n", path);
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
	ionic::TableOptions options;
	options.innerHDivider = false;
	ionic::Table table(options);

	table.addRow({ room.name });
	table.addRow({ zone.name });
	if (!room.desc.empty())
		table.addRow({ ionic::Table::normalizeMD(room.desc, 2) });

	table.setCell(0, 0, { ionic::Color::white }, {});
	table.print();
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

static void ConsoleZoneDriver(ScriptAssets& assets, ScriptBridge& bridge, EntityID zone, std::string dir, uint32_t seed)
{
	ZoneDriver driver(assets, bridge, zone, "player");
	driver.mapData.random.setSeed(seed);

	while (!driver.isGameOver()) {
		ZoneDriver::Mode mode = driver.mode();

		if (mode == ZoneDriver::Mode::kText) {
			while (driver.mode() == ZoneDriver::Mode::kText) {
				TextLine tl = driver.text();
				PrintText(tl.speaker, tl.text);
				driver.advance();
			}
			PrintNews(driver.mapData.newsQueue);
		}
		else if (mode == ZoneDriver::Mode::kChoices)
		{
			const Choices& choices = driver.choices();
			PrintChoices(choices);
			fmt::print("> ");
			Value v2 = Value::ParseValue(ReadString());
			if (v2.intInRange((int)choices.choices.size()))
				driver.choose(v2.intVal);
			PrintNews(driver.mapData.newsQueue);
		}
		else if (mode == ZoneDriver::Mode::kNavigation) {
			PrintRoomDesc(driver.currentZone(), driver.currentRoom());
			const Actor& player = driver.getPlayer();
			const ContainerVec& containerVec = driver.getContainers();
			const InteractionVec& interactionVec = driver.getInteractions();
			std::vector<DirEdge> edges = driver.dirEdges();

			PrintInventory(assets.getInventory(player));
			PrintContainers(driver, containerVec);
			PrintInteractions(interactionVec, assets);
			PrintEdges(edges);
			fmt::print("Menu: (/s)ave (/l)oad (/c)ycle (/q)uit\n");

			fmt::print("> ");
			Value v = Value::ParseValue(ReadString());
			if (ProcessMenu(v.rawStr, dir, driver)) {
				break;
			}
			else if (v.charIntInRange('c', (int)containerVec.size())) {
				const Container* c = driver.getContainer(containerVec[v.intVal]->entityID);
				
				ZoneDriver::TransferResult tr = driver.transferAll(*c, player);
				if (tr == ZoneDriver::TransferResult::kLocked)
					PrintTextLine("The container is locked.", ionic::Color::red);
			}
			else if (v.charIntInRange('i', (int)interactionVec.size())) {
				driver.startInteraction(interactionVec[v.intVal]);
			}
			else {
				int dirIdx = SelectEdge(v, edges);
				if (dirIdx >= 0) {
					if (driver.move(edges[dirIdx].dstRoom) == ZoneDriver::MoveResult::kLocked)
						PrintTextLine("That way is locked.", ionic::Color::red);
				}
			}
			if (driver.mode() == ZoneDriver::Mode::kNavigation) {
				PrintNews(driver.mapData.newsQueue);
			}
		}
		else if (mode == ZoneDriver::Mode::kBattle) {
			const Battle& battle = driver.battle();
			VarBinder binder = driver.battleVarBinder();
#if 1
			bool victory = ConsoleBattleDriver(assets, binder, battle, driver.getPlayer().entityID, driver.mapData.random);
#else
			bool victory = false;
#endif
			driver.battleDone();
			if (!victory) {
				driver.endGame("You have been defeated.", -1);
			}
		}
		else {
			assert(false);
			break;
		}
	}
	if (!driver.endGameMsg().empty()) {
		ionic::Color color = ionic::Color::white;
		if (driver.endGameBias() < 0)
			color = ionic::Color::red;
		else if (driver.endGameBias() > 0)
			color = ionic::Color::green;
		PrintTextLine(driver.endGameMsg(), color);
	}
}

std::string SelectGame(const std::vector<std::filesystem::path>& paths)
{
	ionic::TableOptions options;
	options.outerBorder = false;
	options.innerHDivider = false;
	options.textColor = configChoiceColor;
	options.tableColor = configChoiceColor;
	ionic::Table table(options);

	table.addRow({ "0", "Exit"});
	int i = 1;
	for(const auto& path : paths) {
		table.addRow({ std::to_string(i), path.stem().string() });
		i++;
	}
	table.print();
	fmt::print("> ");

	Value v = Value::ParseValue(ReadString());
	int option = v.intVal - 1;
	if (option >= 0 && option < paths.size())
		return paths[option].string();
	return "";
}

static void RunConsoleTests()
{
	RUN_TEST(Value::Test());
	RunConsoleBattleTests();
}

static void RunOutputTests()
{
	static const char* para =
		"You order your usual coffee and sit at a table outside the cafe. The morning sun warms you\n"
		"even though the San Francisco air is cool. The note reads:\n"
		"\n"
		"         \"Cairo. Meet me at the library. Research section in back. - G\"\n"
		"\n"
		"Giselle was with you on the mission to Cairo. A good researcher, a good shot, and a good friend.";
	PrintText("", para);
	printf("******\n");
	PrintText("narrator", para);
	printf("******\n");

	NewsQueue queue;
	Item gold = { "GOLD", "gold" };
	Item silver = { "SILVER", "silver" };
	Item sword = { "SWORD", "sword" };
	Item armor = { "ARMOR", "armor" };

	queue.push(NewsItem::itemDelta(gold, -10, 5));
	queue.push(NewsItem::itemDelta(gold, 10, 15));
	PrintNews(queue);
	printf("******\n");

	Choices choices;
	choices.choices.push_back({ "Go this way" });
	choices.choices.push_back({ "Go that way..." });
	choices.choices.push_back({ "Ponder." });
	PrintChoices(choices);
	printf("******\n");

	Zone zone;
	zone.name = "The Zone";
	Room room;
	room.name = "The Room";
	room.desc = "The description of the room.";
	PrintRoomDesc(zone, room);
	printf("******\n");
	Inventory inv;
	inv.addItem(gold, 10);
	inv.addItem(silver, 5);
	inv.addItem(sword, 1);
	PrintInventory(inv);
	printf("******\n");
}

int main(int argc, const char* argv[])
{
	ionic::Table::initConsole();

	fmt::print("Welcome to the story engine LuRP\n\n");
	int rc = 0;

	argh::parser cmdl;
	cmdl.add_params({ "--seed", "-l", "--log" });
	cmdl.parse(argc, argv);
	std::string scriptFile = cmdl[1];
	std::string startingZone = cmdl[2];

	if (scriptFile.empty()) {
		fmt::print("LuRP story engine. This is the console line runner.\n");
		fmt::print("Usage: lurp <path/to/lua/file> <starting-zone>\n");
		fmt::print("Optional:\n");
		fmt::print("  -l, --log         Set log level: 'error', 'warning', 'info', 'debug'. Default: 'warning'\n");
		fmt::print("  -s, --debugSave   Save everything with warnings.\n");
		fmt::print("  --seed            Random number seed\n");
		fmt::print("  --outputTests     Run output tests\n");
		fmt::print("  --noScan          Do not scan for games or display menu\n");
	}

	bool debugSave = cmdl[{ "-s", "--debugSave" }];
	bool outputTests = cmdl[{ "--outputTests" }];
	bool doScan = !cmdl[{ "--noScan" }];

	uint32_t seed = Random::getTime();
	cmdl({ "--seed" }, seed) >> seed;
	std::string log = "warning";
	cmdl({ "-l", "--log" }, log) >> log;

	std::string dir = GameFileToDir(scriptFile);
	std::string savePath = SavePath(dir, "saves");
	std::string logPath = LogPath();

	plog::Severity logLevel = plog::warning;
	if (log == "error")
		logLevel = plog::error;
	else if (log == "info")
		logLevel = plog::info;
	else if (log == "debug")
		logLevel = plog::debug;

	static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	static plog::RollingFileAppender<plog::TxtFormatter> fileAppender(logPath.c_str(), 1'000'000, 3);
	plog::init(logLevel, &consoleAppender).addAppender(&fileAppender);

	PLOG(plog::info) << "Logging started.";
	PLOG(plog::info) << "Save path: " << savePath;

#if defined(_DEBUG) && defined(_WIN32)
	// plog::init throws off memory tracking.
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1);
#endif
	{
		std::string gameFile = scriptFile; // prevents a false positive in the leak check.
		{
			RunTests();
			RunConsoleTests();
			rc = TestReturnCode();
			LogTestResults();
			if (rc == 0)
				PLOG(plog::info) << "LuRP tests run successfully.";
			else
				PLOG(plog::error) << "LuRP tests failed.";
		}
		if (outputTests) {
			RunOutputTests();
			BattleOutputTests();
		}
		Globals::debugSave = debugSave;

		if (doScan && gameFile.empty()) {
			auto gameList = ScanGameFiles();
			if (!gameList.empty())
				gameFile = SelectGame(gameList);
		}

		// Run game.
		if (!gameFile.empty()) {
			ScriptBridge bridge;
			ConstScriptAssets csassets = bridge.readCSA(gameFile);
			ScriptAssets assets(csassets);

			ConsoleZoneDriver(assets, bridge, startingZone, dir, seed);
		}
	}

#if defined(_DEBUG) && defined(_WIN32)
	int knownNumLeak = 0;
	int knownLeakSize = 0;

	_CrtMemCheckpoint(&s2);
	_CrtMemDifference(&s3, &s1, &s2);
	_CrtMemDumpStatistics(&s3);

	auto leakCount = s3.lCounts[1];
	auto leakSize = s3.lSizes[1];
	auto totalAllocaton = s3.lTotalCount;
	auto highWater = s3.lHighWaterCount / 1024;
	plog::Severity severity = leakCount ? plog::warning : plog::info;

	PLOG(severity) << "Memory report:";
	PLOG(severity) << "Leak count = " << leakCount << " size = " << leakSize;
	PLOG(severity) << "High water = " << highWater << " total allocated = " << totalAllocaton;
	assert(s3.lCounts[1] <= knownNumLeak);
	assert(s3.lSizes[1] <= knownLeakSize);
#endif
	return rc;
}
