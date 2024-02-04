#include "test.h"
#include "zonedriver.h"
#include "scriptdriver.h"
#include "scriptasset.h"
#include "scripthelper.h"
#include "scriptbridge.h"
#include "battle.h"
#include "tree.h"

#include <fmt/core.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace lurp::swbattle;
using namespace lurp;

int gNTestPass = 0;
int gNTestFail = 0;
int gNRunTest = 0;

void IncTestRun() { ++gNRunTest; }

void RecordTest(bool pass)
{
	if (pass)
		++gNTestPass;
	else
		++gNTestFail;
}

void LogTests()
{
	if (gNTestPass == 0) {
		fmt::print("ERROR No tests ran!\n");
	}
	else {
		fmt::print("RUN_TEST: {} TEST passed: {}\n", gNRunTest, gNTestPass);
	}
}

/*
static void PrintForest(const ScriptAssets& assets, const std::vector<NodeRef>& tree)
{
	for (const NodeRef& ref : tree) {
		fmt::print("{: >{}}", "", 2 + ref.depth * 2);
		std::string name = scriptTypeName(ref.ref.type);
		std::string id;
		if (ref.ref.type == ScriptType::kScript)
			id = assets.scripts[ref.ref.index].entityID;
		else if (ref.ref.type == ScriptType::kChoices)
			id = assets.choices[ref.ref.index].entityID;
		fmt::print("{}: {} {} {} {}\n", ref.depth, name, ref.ref.index, id, ref.leading ? "L" : "T");
	}
}
*/

static void BasicTest(const ConstScriptAssets& ca)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, nullptr, "testplayer");

	map.setZone("TestZone0", "TestZone0_ROOM_A");
	TEST(map.currentRoom().name == "RoomA");
	TEST(map.dirEdges().size() == 1);
	TEST(map.dirEdges()[0].dstRoom == "TestZone0_ROOM_B");

	// Teleport to B and back to A
	map.teleport("TestZone0_ROOM_B");
	TEST(map.currentRoom().name == "RoomB");
	map.teleport("TestZone0_ROOM_A");
	TEST(map.currentRoom().name == "RoomA");

	// Do a proper walk from A to B
	// 1. check door is lock
	TEST(map.mapData.newsQueue.empty());
	auto moveResult = map.move("TestZone0_ROOM_B");
	TEST(moveResult == ZoneDriver::MoveResult::kLocked);
	TEST(map.currentRoom().name == "RoomA");
	TEST(map.mapData.newsQueue.empty());

	// 2. open the chest and get the key
	ContainerVec containerVec = map.getContainers();
	TEST(containerVec.size() == 1);
	const Container& chest = *containerVec[0];
	Inventory& chestInv = assets.inventories.at(chest.entityID);

	const Item& key01 = assets.getItem("KEY_01");
	TEST(chestInv.hasItem(key01) == true);
	map.transfer(key01, chest, map.getPlayer());
	TEST(chestInv.hasItem(key01) == false);
	TEST(map.mapData.newsQueue.size() == 1);
	map.mapData.newsQueue.pop();

	// 3. unlock the door, go to RoomB
	TEST(map.mapData.newsQueue.empty());
	moveResult = map.move("TestZone0_ROOM_B");
	TEST(moveResult == ZoneDriver::MoveResult::kSuccess);
	TEST(map.currentRoom().name == "RoomB");
	TEST(map.mapData.newsQueue.size() == 1);
	NewsItem ni = map.mapData.newsQueue.pop();
	TEST(ni.type == NewsType::kUnlocked);
	TEST(ni.item->entityID == "KEY_01");
	TEST(map.mapData.newsQueue.empty());

	// 4. back to RoomA
	moveResult = map.move("TestZone0_ROOM_A");
	TEST(moveResult == ZoneDriver::MoveResult::kSuccess);
	TEST(map.currentRoom().name == "RoomA");
}


static void DialogTest_Bookcase(const ConstScriptAssets& ca, const EntityID& dialog, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);

	ScriptEnv env = { dialog, NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver dd(assets, env, coreData, &bridge);

	const ScriptHelper* runner = dd.getHelper();

	TEST(!dd.done());
	TEST(dd.type() == ScriptType::kText);
	TEST(dd.line().speaker == "narrator");
	TEST(dd.line().text == "You see a bookcase.");
	dd.advance();
	TEST(dd.type() == ScriptType::kText);
	TEST(dd.line().speaker == "narrator");
	TEST(dd.line().text == "The books are in a strange language.");
	dd.advance();
	TEST(dd.type() == ScriptType::kChoices);
	TEST(dd.choices().choices[0].text == "Read a book");
	dd.choose(0);
	TEST(dd.type() == ScriptType::kText);
	dd.advance();
	TEST(dd.type() == ScriptType::kChoices);
	TEST(dd.choices().choices[1].text == "Read the arcane book");
	TEST(runner->get("player.arcane").type == LUA_TNIL);
	dd.choose(1);
	TEST(runner->get("player.arcane").type == LUA_TBOOLEAN);
	TEST(runner->get("player.arcane").boolean == true);
	TEST(dd.type() == ScriptType::kText);
	TEST(dd.line().text == "You have an arcane glow.");
	dd.advance();
	TEST(dd.type() == ScriptType::kText);
	TEST(dd.line().text == "You step away from the bookcase.");
	TEST(!dd.done());
	dd.advance();
	TEST(dd.done());
}

static void TestInventory1()
{
	Inventory inv;
	Item book = { "ITEM_BOOK", "book" };
	Item potion = { "ITEM_POTION", "potion" };

	TEST(inv.findItem(book) == 0);
	TEST(inv.hasItem(book) == false);
	TEST(inv.hasItem(book) == false);
	inv.addItem(book);
	TEST(inv.findItem(book) == 1);
	TEST(inv.hasItem(book) == true);
	TEST(inv.hasItem(book) == true);
	inv.removeItem(book);
	TEST(inv.findItem(book) == 0);
	TEST(inv.hasItem(book) == false);
	TEST(inv.hasItem(book) == false);
}

static void TestInventory2()
{
	Inventory inv;
	Item potion = { "ITEM_POTION", "potion" };

	TEST(inv.hasItem(potion) == false);
	TEST(inv.findItem(potion) == 0);
	inv.addItem(potion, 2);
	TEST(inv.hasItem(potion) == true);
	TEST(inv.findItem(potion) == 2);
	inv.addItem(potion, 3);
	TEST(inv.hasItem(potion) == true);
	TEST(inv.findItem(potion) == 5);
	inv.removeItem(potion, 1);
	TEST(inv.hasItem(potion) == true);
	TEST(inv.findItem(potion) == 4);
	inv.removeItem(potion, 4);
	TEST(inv.hasItem(potion) == false);
	TEST(inv.findItem(potion) == 0);
}

static void TestScriptAccess()
{
	ScriptBridge bridge;
	ConstScriptAssets csa= bridge.readCSA("");
	ScriptAssets assets(csa);
	CoreData coreData(&bridge);
	ScriptEnv env = { NO_ENTITY, NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptHelper runner(bridge, coreData, env);

	ScriptRef ref = assets.get("testplayer");
	TEST(ref.type == ScriptType::kActor);
	const Actor& player = assets.actors[ref.index];
	TEST(player.name == "Test Player");

	TEST(runner.get("player.maxHP").num == 10.0);
	TEST(runner.get("player.maxHP").num == 10.0);
	TEST(runner.get("player.hp").num == 9.0);
	runner.set("player.hp", Variant(runner.get("player.maxHP")));
	TEST(runner.get("player.hp").num == 10.0);
}

static void TestLoad(bool inner)
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("");
	ScriptAssets assets(ca);
	ZoneDriver map(assets, &bridge, "testplayer");

	{
		ScriptBridge loader;
		loader.loadLUA("save.lua");
		EntityID scriptID = map.load(loader);

		TEST(!scriptID.empty());
		
		if (!scriptID.empty()) {
			ScriptDriver driver(assets, scriptID, map.mapData, bridge, loader);

			TEST(driver.valid());
			if (!inner) {
				TEST(driver.getTree()->size() > 0);
				TEST(driver.type() == ScriptType::kText);
			}
			else {
				TEST(driver.type() == ScriptType::kChoices);
				TEST(driver.choices().choices[0].text == "Read another book");
			}

			driver.clear();
		}
	}

	TEST(map.mapData.coreData.coreGet("testplayer", "attrib.str").second.num == 10.0);
	TEST(map.mapData.coreData.coreGet("testplayer", "name").second.str == "Gromm");
}

static void TestSave(const ConstScriptAssets& ca, ScriptBridge& bridge, bool inner)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, &bridge, "testplayer");

	map.mapData.coreData.coreSet("testplayer", "attrib.str", Variant(10.0), false);
	map.mapData.coreData.coreSet("testplayer", "name", Variant("Gromm"), false);

	map.setZone("TestZone0", "TestZone0_ROOM_A");
	ContainerVec containerVec = map.getContainers();
	const Container* chest = map.getContainer(containerVec[0]->entityID);
	Inventory& chestInventory = assets.inventories.at(chest->entityID);

	const Item& key01 = assets.getItem("KEY_01");
	TEST(chestInventory.hasItem(key01) == true);
	
	const Actor& player = assets.actors[assets.get("testplayer").index];
	Inventory& playerInventory = assets.inventories.at(player.entityID);

	transfer(key01, chestInventory, playerInventory);
	TEST(chestInventory.hasItem(key01) == false);

	map.move("TestZone0_ROOM_B");
	TEST(map.currentRoom().name == "RoomB");

	const InteractionVec& interactionVec = map.getInteractions();
	TEST(interactionVec.size() == 3);
	const Interaction* interaction = interactionVec[0];
	ScriptEnv env = map.getScriptEnv(interaction);

	// DIALOG_BOOKCASE_V1
	ScriptDriver driver(assets, env, map.mapData, &bridge);
	TEST(driver.type() == ScriptType::kText);

	if (inner) {
		driver.advance(); 
		driver.advance();
		TEST(driver.type() == ScriptType::kChoices);
		driver.choose(0);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.type() == ScriptType::kChoices);
	}

	std::ofstream stream;
	stream.open("save.lua", std::ios::out);
	assert(stream.is_open());

	// - coreData
	// - assets
	// - read list
	// - current map / script
	//
	map.save(stream);
	driver.save(stream);

	stream.close();
	driver.clear();	// since we are not finishing the script
}

static void TestCodeEval()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("");
	ScriptAssets assets(csa);
	MapData coreData(&bridge);
	
	ScriptEnv env = { "TEST_MAGIC_BOOK", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };

	{
		{
			ScriptHelper runner(bridge, coreData.coreData, env);
			runner.set("player.class", Variant("fighter"));
			runner.set("player.arcane", Variant());
			runner.set("player.mystery", Variant());
		}
		ScriptDriver driver(assets, env, coreData, &bridge);
		TEST(driver.type() == ScriptType::kChoices);
		TEST(driver.choices().choices.size() == 1);
		driver.choose(0);
		TEST(driver.done());
	}
	{
		{
			ScriptHelper runner(bridge, coreData.coreData, env);
			runner.set("player.class", Variant("druid"));
			runner.set("player.arcane", Variant());
			runner.set("player.mystery", Variant());
		}

		ScriptDriver driver(assets, env, coreData, &bridge);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}
	{
		{
			ScriptHelper runner(bridge, coreData.coreData, env);
			// Set up a run
			runner.set("player.class", Variant("wizard"));
			runner.set("player.arcane", Variant());
			runner.set("player.mystery", Variant());
		}

		ScriptDriver driver(assets, env, coreData, &bridge);
		const ScriptHelper* runner = driver.getHelper();
		TEST(driver.type() == ScriptType::kText);
		TEST(runner->get("player.mystery").type == LUA_TBOOLEAN);
		TEST(runner->get("player.mystery").boolean == true);
		driver.advance();
		TEST(driver.type() == ScriptType::kChoices);
		TEST(driver.choices().choices.size() == 1);
		driver.choose(0);
		TEST(driver.type() == ScriptType::kText);
		TEST(runner->get("player.arcane").boolean == true);
		driver.advance();
		TEST(driver.done());
	}
}

static void TestBattle()
{
	TEST_FP(BattleSystem::chance(1, Die(1, 4, 0)), 0.75);	// rolling 1 is always failure
	TEST_FP(BattleSystem::chance(2, Die(1, 4, 0)), 0.75);
	TEST_FP(BattleSystem::chance(3, Die(1, 4, 0)), 0.50);
	TEST_FP(BattleSystem::chance(4, Die(1, 4, 0)), 0.25);
	TEST_FP(BattleSystem::chance(5, Die(1, 4, 0)), 0.25);
	TEST_FP(BattleSystem::chance(8, Die(1, 4, 0)), 0.25 * 0.25);
	TEST_FP(BattleSystem::chance(8, Die(1, 8, 0)), 0.125);
	TEST_FP(BattleSystem::chance(4, Die(1, 4, 1)), 0.50);

	// 1/2, 1/5. chance both succeed = 1/10.
	// fail: 1/2, 4/5. chance both fail = 4/10. chance at least one succeeds = 6/10.
	TEST_FP(BattleSystem::chanceAorBSucceeds(0.5, 0.2), 0.6);

	{
		Random random(17);
		static const int N = 10'000;
		int buckets[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		for (int i = 0; i < N; ++i) {
			Die die(1, 6, 0);
			int v = die.roll(random, nullptr);
			if (v <= 12)
				++buckets[v - 1];
			else
				++buckets[12 - 1];
		}
		for (int i = 1; i < 6; ++i)
			TEST(within(buckets[i - 1], (N / 6) / 2, (N / 6) * 2));
		TEST(buckets[6 - 1] == 0);
		for (int i = 7; i <= 12; ++i)
			TEST(within(buckets[i - 1], (N / 36) / 2, (N / 36) * 2));
	}

	Random random(0xabc);
	BattleSystem system(random);

	// Ancient Temple Battle
	//   statues - flat courtyard - central stone ring
	// Old West Bar
	//   upturned tables - floor - bar
	// Dungeon Hallway

	std::vector<Region> regions{
		{ "Statues", 0, Cover::kMediumCover },
		{ "Overgrown guarden", 8, Cover::kLightCover},
		{ "Pillar Ring", 12, Cover::kMediumCover},
		{ "Crumbling Walls", 20, Cover::kMediumCover }
	};
	system.setBattlefield("Temple Ruins");
	for (const Region& r : regions)
		system.addRegion(r);
}

static void TestScriptBridge(ScriptBridge& engine)
{
	const std::string gtable = "_TestGlobalTable";
	const std::vector<std::string> strArr = { "str1", "str2", "str3" };
	const std::vector<ScriptBridge::StringCount> strCountArr = { { "str1", 1 }, { "str2", 2 }, { "str3", 3 } };
	const std::vector<int> intArr = { 1, 2, 3 };
	{
		engine.pushNewGlobalTable(gtable); 
		{
			engine.setStrField("strField", "strValue");
			engine.setIntField("intField", 42);
			engine.setBoolField("boolField", true);
			engine.setStrArray("strArr", strArr);
			engine.setStrCountArray("strCountArr", strCountArr);
			engine.setIntArray("intArr", intArr);

			engine.pushNewTable("subTable");
			{
				engine.setStrField("subStrField", "subStrValue");
				engine.setIntField("subIntField", 24);
				engine.setBoolField("subBoolField", false);
				engine.setStrArray("subStrArr", strArr);
				engine.setStrCountArray("subStrCountArr", strCountArr);
				engine.setIntArray("subIntArr", intArr);
			}
			engine.pop();
			engine.pushNewTable("", 1);
			{
				engine.setIntArray("indexedSubIntArr", intArr);
			}
			engine.pop();
		}
		engine.pop();
	}
	{
		engine.pushGlobal(gtable);
		{
			TEST(engine.getStrField("strField", {}) == "strValue");
			TEST(engine.getIntField("intField", {}) == 42);
			TEST(engine.getBoolField("boolField", {}) == true);
			TEST(engine.getStrArray("strArr") == strArr);
			TEST(engine.getStrCountArray("strCountArr") == strCountArr);
			TEST(engine.getIntArray("intArr") == intArr);

			engine.pushTable("subTable");
			{
				TEST(engine.getStrField("subStrField", {}) == "subStrValue");
				TEST(engine.getIntField("subIntField", {}) == 24);
				TEST(engine.getBoolField("subBoolField", {}) == false);
				TEST(engine.getStrArray("subStrArr") == strArr);
				TEST(engine.getStrCountArray("subStrCountArr") == strCountArr);
				TEST(engine.getIntArray("subIntArr") == intArr);
			}
			engine.pop();
			engine.pushTable("", 1);
			{
				TEST(engine.getIntArray("indexedSubIntArr") == intArr);
			}
			engine.pop();
		}
		engine.pop();
	}
}

static void TestTextSubstitution(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "_TEST_READING", NO_ENTITY, NO_ENTITY, "testplayer", "_TEST_BOOK_READER" };
	ScriptDriver driver(assets, env, coreData, &bridge);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "Velma is reading a book. She is wearing glasses.");
	driver.advance();
	TEST(driver.line().text == "Test Player smiles at her.");
	driver.advance();
	TEST(driver.line().text == "Finished with the book, Velma closes the novel.");
	driver.advance();
	TEST(driver.line().text == "Read the first book! (Books read = 1)");
	driver.advance();
	TEST(driver.done());
}

static void TestTextTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	Random r(217);
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "_TEST_TEXT_TEST", NO_ENTITY, NO_ENTITY, "testplayer", "_TEST_BOOK_READER" };
	ScriptDriver driver(assets, env, coreData, &bridge);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "This is pathA");
	driver.advance();
	TEST(driver.line().text == "This is NOT pathB");
	driver.advance();
	TEST(driver.done());
}

static void CallScriptTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "_OPEN_RED_CHEST", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver driver(assets, env, coreData, &bridge);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().speaker == "narrator");
	TEST(driver.line().text == "You see a chest with a red gem in the center.");
	driver.advance();
	TEST(driver.done());
}

static void ChoiceMode1RepeatTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "CHOICE_MODE_1_REPEAT", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver driver(assets, env, coreData, &bridge);

	TEST(driver.type() == ScriptType::kText);
	driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 4);
	TEST(driver.choices().choices[0].text == "Choice 0");
	driver.choose(0);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "about choice 0");
	driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 4);
	TEST(driver.choices().choices[0].text == "Choice 0");

	driver.choose(2);
	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 2);
	TEST(driver.choices().choices[0].text == "C2a");
	driver.choose(0);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "about c2a");
	driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 4);
	TEST(driver.choices().choices[0].text == "Choice 0");

	driver.choose(3);
	TEST(driver.type() == ScriptType::kText);
	driver.advance();
	TEST(driver.done());
}

static void ChoiceMode1RewindTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "CHOICE_MODE_1_REWIND", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver driver(assets, env, coreData, &bridge);

	//PrintForest(assets, createTree(assets, "testplayer"));
	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "Text before")
		driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 4);
	TEST(driver.choices().choices[0].text == "Choice 0");
	driver.choose(0);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "about choice 0");
	driver.advance();

	// And now the rewind...
	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "Text before");
	driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 4);
	TEST(driver.choices().choices[0].text == "Choice 0");
	driver.choose(3);

	TEST(driver.type() == ScriptType::kText);
	driver.advance();
	TEST(driver.done());
}

static void ChoiceMode1PopTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "CHOICE_MODE_1_POP", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver driver(assets, env, coreData, &bridge);

	//PrintForest(assets, createTree(assets, "testplayer"));
	TEST(driver.type() == ScriptType::kText);
 	TEST(driver.line().text == "Text before")
		driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 4);
	TEST(driver.choices().choices[0].text == "Choice 0");
	driver.choose(0);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "about choice 0");
	driver.advance();

	// And now the pop...
	TEST(driver.done());
}

static void ChoiceMode2Test(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	MapData coreData(&bridge);
	ScriptEnv env = { "CHOICE_MODE_2_TEST", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };

	ScriptDriver driver(assets, env, coreData, &bridge);

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 2);
	TEST(driver.choices().choices[0].text == "Choice 0");
	TEST(driver.allTextRead("CHOICE_MODE_2") == false);
	TEST(driver.choices().choices[1].text == "Choice 1");

	driver.choose(0);
	driver.line();		// actually marks text as read when it is fetched
	driver.advance();

	driver.choose(1);
	driver.line();
	driver.advance();

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 3);
	driver.choose(2);
	TEST(driver.done());
}

static void FlagTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, &bridge, "testplayer");

	{
		ScriptEnv env = {"TEST_SCRIPT_1", "TEST_ZONE_1", "TEST_ROOM_1", "testplayer", "TEST_ACTOR_1"};
		ScriptDriver driver(assets, env, map.mapData, &bridge);

		TEST(driver.type() == ScriptType::kText);
		//map.coreData.dump();
		TEST(map.mapData.coreData.coreGet("TEST_ACTOR_1", "npcFlag").second.boolean == true);
		TEST(map.mapData.coreData.coreGet("_ScriptEnv", "scriptFlag").second.boolean == true);
		TEST(map.mapData.coreData.coreGet("_ScriptEnv", "npcName").second.str == "TestActor1");
		driver.advance();
		TEST(driver.done());
	}
	TEST(map.mapData.coreData.coreGet("TEST_ACTOR_1", "npcFlag").second.boolean == true);
	TEST(map.mapData.coreData.coreGet("_ScriptEnv", "scriptFlag").second.type == LUA_TNIL);
}

static void TestInventoryScript(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, &bridge, "testplayer");
	ScriptEnv env = { "KEY_MASTER", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver driver(assets, env, map.mapData, &bridge);

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 2);
	TEST(driver.choices().choices[0].text == "Get the key");
	driver.choose(0);
	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "You have the Skeleton Key.");
	driver.advance();
	TEST(driver.done());

	TEST(map.numItems("testplayer", "SKELETON_KEY") == 1);
}

static void TestWalkabout(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver zone(assets, &bridge, "testplayer");
	zone.setZone("TestZone0", "TestZone0_ROOM_A");
	zone.move("TestZone0_ROOM_B");
	// failed:
	TEST(zone.currentRoom().entityID == "TestZone0_ROOM_A");
	Inventory& inv = assets.getInventory(zone.getPlayer());
	inv.addItem(assets.getItem("KEY_01"));
	zone.move("TestZone0_ROOM_B");
	// success:
	TEST(zone.currentRoom().entityID == "TestZone0_ROOM_B");

	// Push the move button:
	{
		const Interaction* iAct = zone.getInteractions()[1];
		TEST(iAct->name == "Move Button");
		ScriptEnv env = zone.getScriptEnv(iAct);
		ScriptDriver driver(assets, env, zone.mapData, &bridge);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}
	TEST(zone.currentRoom().entityID == "TestZone0_ROOM_A");
	zone.move("TestZone0_ROOM_B");

	// Push the teleport button:
	{
		const Interaction* iAct = zone.getInteractions()[2];
		TEST(iAct->name == "Teleport Button");
		ScriptEnv env = zone.getScriptEnv(iAct);
		ScriptDriver driver(assets, env, zone.mapData, &bridge);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}

	TEST(zone.currentZone().entityID == "TEST_ZONE_1");
	TEST(zone.currentRoom().entityID == "TEST_ROOM_1");
}

void TestLuaCore()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("");
	ScriptAssets assets(csa);
	ZoneDriver zoneDriver(assets, &bridge, "testplayer");
	ScriptEnv env = { "TEST_LUA_CORE", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	ScriptDriver driver(assets, env, zoneDriver.mapData, &bridge);

	TEST(driver.type() == ScriptType::kText);
	CoreData& cd = zoneDriver.mapData.coreData;
	TEST(cd.coreGet("ACTOR_01", "STR").second.num == 17.0);
	TEST(cd.coreGet("ACTOR_01", "attributes").first == false);
	cd.coreSet("ACTOR_01", "STR", Variant(18.0), false);

	driver.advance();
	TEST(driver.done());
	TEST(cd.coreGet("ACTOR_01", "STR").second.num == 18.0);

	// Test the path works the same as the Core
	TEST(driver.helper()->get("ACTOR_01.STR").num == 18.0);
	TEST(driver.helper()->get("ACTOR_01.DEX").num == 10.0);
	TEST(driver.helper()->get("ACTOR_01.attributes.sings").boolean == true);
	TEST(driver.helper()->get("player.name").str == "Test Player");

#if 0
	// Save to a file for debugging
	std::ofstream stream;
	stream.open("save2.lua", std::ios::out);
	assert(stream.is_open());
	zoneDriver.save(stream);
	stream.close();
#endif

	std::ostringstream buffer;
	zoneDriver.save(buffer);
	std::string str = buffer.str();

	TEST(str.find("STR") != std::string::npos);
	TEST(str.find("DEX") == std::string::npos);

}

void TestCombatant()
{
	{
		Combatant c;
		c.autoLevel(8, 4, 2, 1);
		TEST(c.fighting.d > 4);
		TEST(c.fighting.d >= c.shooting.d);
		TEST(c.shooting.d >= c.arcane.d);
	}
	{
		Combatant c;
		c.autoLevel(8, 4, 2, 0);
		TEST(c.fighting.d > 4);
		TEST(c.fighting.d >= c.shooting.d);
		TEST(c.arcane.d == 4 && c.arcane.b == -2);
	}
	{
		Combatant c;
		c.autoLevel(8, 2, 4, 0);
		TEST(c.shooting.d > 4);
		TEST(c.shooting.d >= c.fighting.d);
		TEST(c.arcane.d == 4 && c.arcane.b == -2);
	}
	{
		Combatant c;
		c.autoLevel(8, 1, 1, 8);
		TEST(c.arcane.d > 4);
		TEST(c.arcane.d >= c.fighting.d);
		TEST(c.arcane.d >= c.shooting.d);
	}
	{
		Random random(123);
		// The basics...
		BattleSystem system(random);
		system.setBattlefield("Field");
		system.addRegion({ "End", 100, Cover::kNoCover });
		system.addRegion({ "Start", 0, Cover::kNoCover });
		system.addRegion({ "Middle", 0, Cover::kNoCover });

		Combatant a;
		Combatant b;
		system.addCombatant(a);
		system.addCombatant(b);

		TEST(system.combatants().size() == 2);
		TEST(system.regions().size() == 3);
		TEST(system.distance(0, 2) == 100);

		a = system.combatants()[0];
		b = system.combatants()[1];
		TEST(a.index == 0);
		TEST(a.team == 0);
		TEST(a.region == 0);

		TEST(b.index == 1);
		TEST(b.team == 1);
		TEST(b.region == 2);
	}
}

class BattleTest {
public:
	static void Test1();
};

void BattleTest::Test1()
{
	using namespace lurp::swbattle;

	Random random(100);
	BattleSystem system(random);

	system.setBattlefield("Landing Pad");
	system.addRegion({ "Mooring", 0, Cover::kLightCover });
	system.addRegion({ "Catwalk", 10, Cover::kNoCover });
	system.addRegion({ "Dock", 20, Cover::kMediumCover });

	RangedWeapon blaster = { "blaster", {2, 6, 0}, 2, 4, 30 };
	MeleeWeapon baton = { "baton", {1, 4, 0}, 4, false };
	Armor riotGear = { "riot gear", 2, 6 };
	Power starCharm = { ModType::kBoost, "StarCharm", 3, 1, 1 };
	Power farCharm = { ModType::kBoost, "FarCharm", 3, 4, 1 };
	Power swol = { ModType::kStrength, "Swol", 1, 1, 1 };
	Power spark = { ModType::kBolt, "Spark", 1, 1, 1 };

	Combatant a;
	a.name = "Rogue Rebel";
	a.wild = true;
	a.autoLevel(8, 2, 4, 2, 0);
	a.rangedWeapon = blaster;
	a.powers.push_back(starCharm);
	a.powers.push_back(farCharm);

	Combatant b;
	b.name = "Brute";
	b.autoLevel(6, 4, 2, 0, 0);
	b.meleeWeapon = baton;	
	b.armor = riotGear;

	Combatant c;
	c.name = "Sparky";
	c.autoLevel(6, 0, 2, 4, 0);
	c.rangedWeapon = blaster;
	c.powers.push_back(spark);
	c.powers.push_back(swol);

	system.addCombatant(a);
	system.addCombatant(b);
	system.addCombatant(c);

	system.start(false);
	TEST(system.queue.empty());
	TEST(system.turn() == 0);

	TEST(system.checkAttack(0, 1) == BattleSystem::ActionResult::kSuccess);
	// Brute has no ranged weapon:
	TEST(system.checkAttack(1, 0) == BattleSystem::ActionResult::kInvalid);
	// Can't attack allies:
	TEST(system.checkAttack(1, 2) == BattleSystem::ActionResult::kInvalid);

	TEST(system.checkMove(0, 1) == BattleSystem::ActionResult::kSuccess);
	TEST(system.checkMove(0, -1) == BattleSystem::ActionResult::kInvalid);

	TEST(system.checkPower(0, 1, 0) == BattleSystem::ActionResult::kOutOfRange);	// StarCharm out of range
	TEST(system.checkPower(0, 1, 1) == BattleSystem::ActionResult::kInvalid);		// FarCharm is for allies
	TEST(system.checkPower(0, 0, 0) == BattleSystem::ActionResult::kSuccess);

	std::vector<ModInfo> mods;
	std::pair<Die, int> p = system.calcRanged(0, 1, mods);
	TEST(p.first == Die(1, 6, -6));
	TEST(p.second == 4);
	TEST(mods.size() == 2);
	TEST(mods[0].type == ModType::kRange);
	TEST(mods[1].type == ModType::kNaturalCover);
	mods.clear();

	system.place(0, 2);
	p = system.calcMelee(0, 1, mods);
	TEST(p.first == Die(1, 6, 0));
	TEST(p.second == 6);
	TEST(mods.size() == 0);

	system.place(0, 0);
	TEST(system.attack(0, 1) == BattleSystem::ActionResult::kSuccess);
	TEST(system.queue.size() == 1);
	const Action& action = system.queue.front();
	TEST(action.type == Action::Type::kAttack);
	const AttackAction& aa = std::get<AttackAction>(action.data);
	TEST(aa.attacker == 0);
	TEST(aa.defender == 1);
	TEST(aa.attackRoll.die == Die(1, 6, -6));
	TEST(aa.mods.size() == 2);
	TEST(aa.damageMods.size() == 0);
	TEST(aa.success == false);

}

int RunTests()
{
	//fmt::print("sizeof(ConstScriptAssets) = {}\n", sizeof(ConstScriptAssets));

	ScriptBridge bridge;
	ConstScriptAssets csassets = bridge.readCSA("");

	RUN_TEST(BasicTest(csassets));
	RUN_TEST(TestScriptBridge(bridge));
	RUN_TEST(DialogTest_Bookcase(csassets, "DIALOG_BOOKCASE_V1", bridge));
	RUN_TEST(DialogTest_Bookcase(csassets, "DIALOG_BOOKCASE_V2", bridge));
	RUN_TEST(TestInventory1());
	RUN_TEST(TestInventory2());
	RUN_TEST(TestScriptAccess());
	RUN_TEST(TestCodeEval());
	RUN_TEST(TestBattle());
	RUN_TEST(TestTextSubstitution(csassets, bridge));
	RUN_TEST(TestTextTest(csassets, bridge));
	RUN_TEST(CallScriptTest(csassets, bridge));
	RUN_TEST(ChoiceMode1RepeatTest(csassets, bridge));
	RUN_TEST(ChoiceMode1RewindTest(csassets, bridge));
	RUN_TEST(ChoiceMode1PopTest(csassets, bridge));
	RUN_TEST(ChoiceMode2Test(csassets, bridge));
	RUN_TEST(FlagTest(csassets, bridge));
	RUN_TEST(TestSave(csassets, bridge, false));
	RUN_TEST(TestLoad(false));
	RUN_TEST(TestSave(csassets, bridge, true));
	RUN_TEST(TestLoad(true));
	RUN_TEST(TestInventoryScript(csassets, bridge));
	RUN_TEST(TestWalkabout(csassets, bridge));
	RUN_TEST(TestLuaCore());
	RUN_TEST(TestCombatant());
	RUN_TEST(BattleTest::Test1());

	assert(gNTestPass > 0);
	assert(gNTestFail == 0);
	return TestReturnCode();
}

int TestReturnCode()
{
	if (gNTestPass == 0) return -1;
	return gNTestFail;
}
