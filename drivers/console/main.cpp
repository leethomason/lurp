#include "scriptbridge.h"
#include "zonedriver.h"
#include "scriptdriver.h"
#include "test.h"
#include "consoleutil.h"

#include "consolebattle.h"
#include "scriptasset.h"

#include "../platform.h"

// Console driver includes
#include "argh.h"
#include "crtdbg.h"
#include <fmt/core.h>
#include <tabulate/tabulate.hpp>
#include <tabulate/table.hpp>

// C++
#include <stdio.h>
#include <assert.h>

#include <string>
#include <vector>
#include <algorithm>
#include <array>

using namespace lurp;

static constexpr int configSpeakerWidth = 12;
static constexpr tabulate::Color configTextColor = tabulate::Color::yellow;
static constexpr tabulate::Color configChoiceColor = tabulate::Color::cyan;

static const char* doubleNL(const char* p, const char* end)
{
	if (p + 1 < end && p[0] == '\n' && p[1] == '\n')
		return p + 2;

	int nl = 0;
	int cr = 0;
	if (p + 3 < end) {
		for(int i=0; i<4; i++) {
			if (p[i] == '\n')
				nl++;
			if (p[i] == '\r')
				cr++;
		}
	}
	if (nl == 2 && cr == 2)
		return p + 4;
	return nullptr;
}

static const char* skipWS(const char* p, const char* end)
{
	if (!isspace(*p))
		return nullptr;

	while (p < end && isspace(*p))
		p++;
	return p;
}

static void StreamText(const std::string& s, const std::string& speaker, tabulate::Table& table)
{
	// Trying to get all the text rules right - so very difficult.
	// Especially since the text coming in could be a long string literal (without new lines)
	// or new line separated strings. And we want to handle both. And the \n \r mess.
	// Try - just try - to treat all whitespace as a space.
	// There are exceptions, of course.

	std::string buf;
	buf.reserve(s.size());

	const char* p = s.c_str();
	const char* end = p + s.size();

	while (p < end) {
		const char* q = doubleNL(p, end);
		if (q) {
			if (speaker.empty())
				table.add_row({ buf });
			else
				table.add_row({ speaker, buf });
			buf.clear();
			p = q;
			continue;
		}
		q = skipWS(p, end);
		if (q) {
			buf += ' ';
			p = q;
		}
		else {
			buf += *p;
			p++;
		}
	}
	// Remove trailing white space.
	if (!buf.empty() && buf.back() == ' ')
		buf.pop_back();

	if (!buf.empty()) {
		if (speaker.empty())
			table.add_row({ buf });
		else
			table.add_row({ speaker, buf });
	}
}

static void PrintTextLine(const std::string& text, tabulate::Color color, bool bold = false)
{
	assert(color != tabulate::Color::grey); // Grey doesn't print. Bug in tabulate?

	int w = ConsoleWidth();
	tabulate::Table table;
	table.format().width(w - 2);
	table.format().border("").corner("").padding(0);
	table.format().padding_bottom(0);
	table.format().color(color);
	if (bold)
		table.format().font_style({ tabulate::FontStyle::bold });
	table.add_row({ text });
	std::cout << table << std::endl;
}

static void PrintText(std::string speaker, const std::string& text)
{
	int w = ConsoleWidth();
	tabulate::Table table;

	table.format().width(w - 2);
	table.format().border("").corner("").padding(0);
	table.format().padding_bottom(1);
	table.format().color(configTextColor);

	//speaker = "foo";
	StreamText(text, speaker, table);
	if (speaker.empty()) {
	}
	else {
		table.column(0).format().width(configSpeakerWidth);
		table.column(1).format().width(w - 3 - configSpeakerWidth);
	}
	std::cout << table << std::endl;
}

static void PrintChoices(const Choices& choices)
{
	//int w = ConsoleWidth();
	tabulate::Table table;
	int i = 0;

	table.format().color(configChoiceColor);

	for (const Choices::Choice& c : choices.choices) {
		table.add_row({ std::to_string(i), c.text });
		i++;
	}
	static constexpr int choiceWidth = 2;
	std::cout << table << std::endl;
}

static void PrintNews(NewsQueue& queue)
{
	while (!queue.empty()) {
		NewsItem ni = queue.pop();
		if (ni.type == NewsType::kItemDelta) {
			assert(ni.item);
			int bias = ni.delta < 0 ? -1 : 1;
			tabulate::Color color = ni.delta < 0 ? tabulate::Color::red : tabulate::Color::green;
			PrintTextLine(
				fmt::format("You {} {} {}", ni.verb(), ni.delta * bias, ni.item->name),
				color
			);
		}
		else if (ni.type == NewsType::kLocked || ni.type == NewsType::kUnlocked) {
			std::string s = fmt::format("The {} was {}",ni.noun(), ni.verb());
			if (ni.item)
				s += fmt::format(" by the {}", ni.item->name);
			PrintTextLine(s, tabulate::Color::green);
		}
	}
}

static void ConsoleScriptDriver(ScriptAssets& assets, ScriptBridge& bridge, const ScriptEnv& env, MapData& mapData)
{
	ScriptDriver dd(assets, env, mapData, &bridge);

	while (!dd.done()) {
		while (dd.type() == ScriptType::kText) {
			TextLine line = dd.line();
			PrintText(line.speaker, line.text);
			dd.advance();
			PrintNews(mapData.newsQueue);
		}
		if (dd.done())
			break;

		if (dd.type() == ScriptType::kChoices) {
			const Choices& choices = dd.choices();
			PrintChoices(choices);
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

static std::vector<std::string> wrapAlgo(const std::vector<std::string>& words, const std::string& sep, int width)
{
	std::vector<std::string> lines;
	std::string line;
	for (const std::string& word : words) {
		if (line.empty()) {
			line = word;
		}
		else if (line.size() + word.size() > width) {
			lines.push_back(line);
			line = word;
		}
		else {
			line += sep + word;
		}
	}
	if (!line.empty()) {
		lines.push_back(line);
	}
	return lines;
}

static void PrintInventory(const Inventory& inv)
{
	if (inv.emtpy()) {
		fmt::print("Inventory empty.\n");
		return;
	}

	int w = ConsoleWidth();
	std::vector<std::string> words;
	for (const auto& itemRef : inv.items()) {
		words.push_back(fmt::format("{}: {}", itemRef.pItem->name, itemRef.count));
	}
	std::vector<std::string> lines = wrapAlgo(words, " | ", w - 4);
	for (const std::string& line : lines) {
		fmt::print("| {} |\n", line);
	}
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
			fmt::print("      ");
			if (!inv.emtpy())
				PrintInventory(inv);
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
	tabulate::Table table;

	table.add_row({ room.name });
	table.add_row({ zone.name });
	if (!room.desc.empty())
		table.add_row({ room.desc });

	table.row(1).format().hide_border_top();
	if (!room.desc.empty())
		table.row(2).format().hide_border_top();

	std::cout << table << std::endl;
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

static void ConsoleZoneDriver(ScriptAssets& assets, ScriptBridge& bridge, EntityID zone, std::string dir)
{
	ZoneDriver driver(assets, &bridge, zone, "player");
	while (true) {
		ZoneDriver::Mode mode = driver.mode();

		if (mode == ZoneDriver::Mode::kText) {
			while (driver.mode() == ZoneDriver::Mode::kText) {
				// FIXME: support speaker mode
				TextLine tl = driver.text();
				//fmt::print("{}\n", tl.text);
				PrintText(tl.speaker, tl.text);
				driver.advance();
			}
			PrintNews(driver.mapData.newsQueue);
		}
		else if (mode == ZoneDriver::Mode::kChoices)
		{
			const Choices& choices = driver.choices();
			PrintChoices(choices);
			Value v2 = Value::ParseValue(ReadString());
			if (v2.intInRange((int)choices.choices.size()))
				driver.choose(v2.intVal);
			PrintNews(driver.mapData.newsQueue);
		}
		else if (mode == ZoneDriver::Mode::kNavigation) {
			if (!driver.endGameMsg().empty())
				break;

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
					PrintTextLine("The container is locked.", tabulate::Color::red);
					//fmt::print("{}Container is locked.{}\n", dye::red, dye::reset);
			}
			else if (v.charIntInRange('i', (int)interactionVec.size())) {
				driver.startInteraction(interactionVec[v.intVal]);
			}
			else {
				int dirIdx = SelectEdge(v, edges);
				if (dirIdx >= 0) {
					if (driver.move(edges[dirIdx].dstRoom) == ZoneDriver::MoveResult::kLocked)
						PrintTextLine("That way is locked.", tabulate::Color::red);
					//fmt::print("{}That way is locked.{}\n", dye::red, dye::reset);
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
		PrintTextLine(driver.endGameMsg(), tabulate::Color::white);
		//fmt::print("{}{}{}\n", dye::white, driver.endGameMsg(), dye::reset);
	}
}

static void RunConsoleTests()
{
	RUN_TEST(Value::Test());
	RunConsoleBattleTests();
}

static void RunOutputTests()
{
	PrintText("narrator", "output");
	printf("******\n");
	static const char* para =
		"You order your usual coffee and sit at a table outside the cafe. The morning sun warms you\n"
		"even though the San Francisco air is cool. The note reads:\n"
		"\n"
		"\"Cairo. Meet me at the library. Research section in back. - G\"\n"
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
	InitConsole();

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
		{
			// fixme: add flag
			//RunOutputTests();
		}
		Globals::trace = trace;
		Globals::debugSave = debugSave;

		if (battleSim) {
			ConsoleBattleSim(era, playerBS, enemyBS, seed);
		}
		else {
			// Run game.
			if (!scriptFile.empty()) {
				ScriptBridge bridge;
				ConstScriptAssets csassets = bridge.readCSA(scriptFile);
				ScriptAssets assets(csassets);

				ScriptRef ref;
				if (argc > 2)
					ref = assets.get(argv[2]);

				if (ref.type == ScriptType::kScript) {
					ScriptEnv env;
					env.script = argv[2];
					env.player = "player";
					MapData mapData(&bridge, 567 + clock());
					ConsoleScriptDriver(assets, bridge, env, mapData);
				}
				else {
					std::string dir = GameFileToDir(scriptFile);
					std::string savePath = SavePath(dir, "saves");
					fmt::print("Save path: {}\n", savePath);
					fmt::print("\n\n");

					ConsoleZoneDriver(assets, bridge, startingZone, dir);
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
