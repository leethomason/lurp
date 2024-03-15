#include "test.h"
#include "zonedriver.h"
#include "scriptdriver.h"
#include "scriptasset.h"
#include "scripthelper.h"
#include "scriptbridge.h"
#include "battle.h"
#include "tree.h"
#include "../drivers/platform.h"

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

void LogTestResults()
{
	if (gNTestPass == 0) {
		PLOG(plog::error) << "No tests ran!";
	}
	else {
		PLOG(plog::info) << "Tests run: " << gNRunTest << " TEST assertions passed: " << gNTestPass;
	}
}

void BridgeWorkingTest(const ScriptBridge& bridge)
{
	TEST(bridge.getLuaState());

}

void HaveCAssetsTest(const ConstScriptAssets& ca)
{
	TEST(ca.zones.size() > 0);
	TEST(ca.scripts.size() > 0);

	ScriptAssets assets(ca);
	TEST(assets.getZone("TEST_ZONE_0").name == "TestDungeon");
}

static void BasicTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, bridge, "testplayer");

	map.setZone("TEST_ZONE_0", "TEST_ZONE_0_ROOM_A");
	TEST(map.currentRoom().name == "RoomA");
	TEST(map.dirEdges().size() == 1);
	TEST(map.dirEdges()[0].dstRoom == "TEST_ZONE_0_ROOM_B");

	// Teleport to B and back to A
	map.teleport("TEST_ZONE_0_ROOM_B");
	TEST(map.currentRoom().name == "RoomB");
	map.teleport("TEST_ZONE_0_ROOM_A");
	TEST(map.currentRoom().name == "RoomA");

	// Do a proper walk from A to B
	// 1. check door is lock
	TEST(map.mapData.newsQueue.empty());
	auto moveResult = map.move("TEST_ZONE_0_ROOM_B");
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
	moveResult = map.move("TEST_ZONE_0_ROOM_B");
	TEST(moveResult == ZoneDriver::MoveResult::kSuccess);
	TEST(map.currentRoom().name == "RoomB");
	TEST(map.mapData.newsQueue.size() == 1);
	NewsItem ni = map.mapData.newsQueue.pop();
	TEST(ni.type == NewsType::kUnlocked);
	TEST(ni.item->entityID == "KEY_01");
	TEST(map.mapData.newsQueue.empty());

	// 4. back to RoomA
	moveResult = map.move("TEST_ZONE_0_ROOM_A");
	TEST(moveResult == ZoneDriver::MoveResult::kSuccess);
	TEST(map.currentRoom().name == "RoomA");
}


static void DialogTest_Bookcase(const ConstScriptAssets& ca, const EntityID& dialog, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver dd(zoneDriver, bridge, dialog);
	VarBinder binder = dd.helper()->varBinder();

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
	TEST(binder.get("player.arcaneGlow").type == LUA_TNIL);
	dd.choose(1);
	TEST(binder.get("player.arcaneGlow").type == LUA_TBOOLEAN);
	TEST(binder.get("player.arcaneGlow").boolean == true);
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

	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "EMPTY_SCRIPT");
	VarBinder binder = driver.varBinder();

	ScriptRef ref = assets.getScriptRef("testplayer");
	TEST(ref.type == ScriptType::kActor);
	const Actor& player = assets._csa.actors[ref.index];
	TEST(player.name == "Test Player");

	TEST(binder.get("player.fighting").num == 4.0);
	binder.set("player.fighting", 5.0);
	TEST(binder.get("player.fighting").num == 5.0);
	binder.set("player.fighting", 4.0);
	TEST(binder.get("player.fighting").num == 4.0);
}

static void TestLoad()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("");
	ScriptAssets assets(ca);

	ZoneDriver map(assets, bridge, "testplayer");

	ScriptBridge loader;
	std::string path = SavePath("test", "testsave");
	loader.loadLUA(path);
	EntityID scriptID = map.load(loader);

	TEST(!scriptID.empty());

	TEST(map.mode() == ZoneDriver::Mode::kChoices);
	TEST(map.choices().choices[0].text == "Read another book");
}

static void TestSave(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, bridge, "testplayer");

	map.setZone("TEST_ZONE_0", "TEST_ZONE_0_ROOM_A");
	ContainerVec containerVec = map.getContainers();
	const Container* chest = map.getContainer(containerVec[0]->entityID);
	Inventory& chestInventory = assets.inventories.at(chest->entityID);

	const Item& key01 = assets.getItem("KEY_01");
	TEST(chestInventory.hasItem(key01) == true);

	const Actor& player = assets._csa.actors[assets.getScriptRef("testplayer").index];
	Inventory& playerInventory = assets.inventories.at(player.entityID);

	transfer(key01, chestInventory, playerInventory);
	TEST(chestInventory.hasItem(key01) == false);

	map.move("TEST_ZONE_0_ROOM_B");
	TEST(map.currentRoom().name == "RoomB");

	const InteractionVec& interactionVec = map.getInteractions();
	TEST(interactionVec.size() == 3);
	const Interaction* interaction = interactionVec[0];
	map.startInteraction(interaction);

	TEST(map.mode() == ZoneDriver::Mode::kText);
	map.advance();
	map.advance();
	TEST(map.mode() == ZoneDriver::Mode::kChoices);
	map.choose(0);
	TEST(map.mode() == ZoneDriver::Mode::kText);
	map.advance();
	TEST(map.mode() == ZoneDriver::Mode::kChoices);

	std::string path = SavePath("test", "testsave");
	std::ofstream stream = OpenSaveStream(path);

	map.save(stream);
	stream.close();
}

static void TestCodeEval()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("");
	ScriptAssets assets(csa);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptEnv env = { "TEST_MAGIC_BOOK", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	
	{
		{
			VarBinder binder(bridge, zoneDriver.mapData.coreData, env);
			binder.set("player.class", Variant("fighter"));
			binder.set("player.arcaneGlow", Variant());
			binder.set("player.mystery", Variant());
		}
		ScriptDriver driver(zoneDriver, bridge, "TEST_MAGIC_BOOK");
		TEST(driver.type() == ScriptType::kChoices);
		TEST(driver.choices().choices.size() == 1);
		driver.choose(0);
		TEST(driver.done());
	}
	{
		{
			VarBinder binder(bridge, zoneDriver.mapData.coreData, env);
			binder.set("player.class", Variant("druid"));
			binder.set("player.arcaneGlow", Variant());
			binder.set("player.mystery", Variant());
		}

		ScriptDriver driver(zoneDriver, bridge, "TEST_MAGIC_BOOK");
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}
	{
		{
			VarBinder binder(bridge, zoneDriver.mapData.coreData, env);
			// Set up a run
			binder.set("player.class", Variant("wizard"));
			binder.set("player.arcaneGlow", Variant());
			binder.set("player.mystery", Variant());
		}

		ScriptDriver driver(zoneDriver, bridge, "TEST_MAGIC_BOOK");
		VarBinder binder = driver.varBinder();
		TEST(driver.type() == ScriptType::kText);
		TEST(binder.get("player.mystery").type == LUA_TBOOLEAN);
		TEST(binder.get("player.mystery").boolean == true);
		driver.advance();
		TEST(driver.type() == ScriptType::kChoices);
		TEST(driver.choices().choices.size() == 1);
		driver.choose(0);
		TEST(driver.type() == ScriptType::kText);
		TEST(binder.get("player.arcaneGlow").boolean == true);
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
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "_TEST_READING");

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
	ScriptAssets assets(ca);
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "_TEST_TEXT_TEST", NO_ENTITY, NO_ENTITY, "testplayer", "_TEST_BOOK_READER" };
	//ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "_TEST_TEXT_TEST");

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
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "_OPEN_RED_CHEST", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "_OPEN_RED_CHEST");

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().speaker == "narrator");
	TEST(driver.line().text == "You see a chest with a red gem in the center.");
	driver.advance();
	TEST(driver.done());
}

static void ChoiceMode1RepeatTest(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "CHOICE_MODE_1_REPEAT", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "CHOICE_MODE_1_REPEAT");

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
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "CHOICE_MODE_1_REWIND", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "CHOICE_MODE_1_REWIND");

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
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "CHOICE_MODE_1_POP", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "CHOICE_MODE_1_POP");

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
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "CHOICE_MODE_2_TEST", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//	ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "CHOICE_MODE_2_TEST");

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
	using Mode = ZoneDriver::Mode;

	ScriptAssets assets(ca);
	ZoneDriver driver(assets, bridge, "TEST_ZONE_1", "testplayer");

	TEST(driver.mode() == Mode::kText);
	//map.coreData.dump();
	TEST(driver.mapData.coreData.coreGet("TEST_ACTOR_1", "npcFlag").second.boolean == true);
	TEST(driver.mapData.coreData.coreGet("_ScriptEnv", "scriptFlag").second.boolean == true);
	TEST(driver.mapData.coreData.coreGet("_ScriptEnv", "npcName").second.str == "TestActor1");
	driver.advance();
	TEST(driver.mode() == Mode::kNavigation);

	TEST(driver.mapData.coreData.coreGet("TEST_ACTOR_1", "npcFlag").second.boolean == true);
	TEST(driver.mapData.coreData.coreGet("_ScriptEnv", "scriptFlag").second.type == LUA_TNIL);
}

static void TestInventoryScript(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver map(assets, bridge, "testplayer");
	//ScriptEnv env = { "KEY_MASTER", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, map.mapData, bridge);
	ScriptDriver driver(map, bridge, "KEY_MASTER");

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

static void TestContainers(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver zone(assets, bridge, "testplayer");
	zone.setZone("TEST_ZONE_2", "TEST_ROOM_2");

	const Actor& player = zone.getPlayer();

	TEST(zone.currentRoom().name == "TestRoom2");
	TEST(zone.getContainers().size() == 2);

	// 1. Makes sure we can't open the locked chest.
	ContainerVec containerVec = zone.getContainers();
	const Container& chestA = *containerVec[0];
	const Container& chestB = *containerVec[1];

	TEST(zone.transferAll(chestA, player) == ZoneDriver::TransferResult::kLocked);
	TEST(zone.mapData.newsQueue.empty());

	TEST(zone.transferAll(chestB, player) == ZoneDriver::TransferResult::kSuccess);
	TEST(zone.mapData.newsQueue.size() == 1); // got a key
	zone.mapData.newsQueue.clear();

	TEST(zone.transferAll(chestA, player) == ZoneDriver::TransferResult::kSuccess);
	TEST(zone.getInventory(player).numItems(assets.getItem("GOLD")) == 100);
	TEST(zone.mapData.newsQueue.size() == 2);	// unlocked and have gold
}

static void TestWalkabout(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	ZoneDriver zone(assets, bridge, "testplayer");
	zone.setZone("TEST_ZONE_0", "TEST_ZONE_0_ROOM_A");
	zone.move("TEST_ZONE_0_ROOM_B");
	// failed:
	TEST(zone.currentRoom().entityID == "TEST_ZONE_0_ROOM_A");
	Inventory& inv = assets.getInventory(zone.getPlayer());
	inv.addItem(assets.getItem("KEY_01"));
	zone.move("TEST_ZONE_0_ROOM_B");
	// success:
	TEST(zone.currentRoom().entityID == "TEST_ZONE_0_ROOM_B");

	// Push the move button:
	{
		const Interaction* iAct = zone.getInteractions()[1];
		TEST(iAct->name == "Move Button");
		ScriptEnv env = zone.getScriptEnv(iAct);
		ScriptDriver driver(zone, bridge, env);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}
	TEST(zone.currentRoom().entityID == "TEST_ZONE_0_ROOM_A");
	zone.move("TEST_ZONE_0_ROOM_B");

	// Push the teleport button:
	{
		const Interaction* iAct = zone.getInteractions()[2];
		TEST(iAct->name == "Teleport Button");
		ScriptEnv env = zone.getScriptEnv(iAct);
		ScriptDriver driver(zone, bridge, env);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}

	TEST(zone.currentZone().entityID == "TEST_ZONE_1");
	TEST(zone.currentRoom().entityID == "TEST_ROOM_1");
}

static void TestLuaCore()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("");
	ScriptAssets assets(csa);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	//ScriptEnv env = { "TEST_LUA_CORE", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, zoneDriver.mapData, bridge);
	ScriptDriver driver(zoneDriver, bridge, "TEST_LUA_CORE");

	TEST(driver.type() == ScriptType::kText);
	CoreData& cd = zoneDriver.mapData.coreData;
	TEST(cd.coreGet("ACTOR_01", "STR").second.num == 17.0);
	TEST(cd.coreGet("ACTOR_01", "attributes").first == false);
	cd.coreSet("ACTOR_01", "STR", Variant(18.0), false);

	driver.advance();
	TEST(driver.done());
	TEST(cd.coreGet("ACTOR_01", "STR").second.num == 18.0);

	// Test the path works the same as the Core
	VarBinder binder = driver.helper()->varBinder();
	TEST(binder.get("ACTOR_01.STR").num == 18.0);
	TEST(binder.get("ACTOR_01.DEX").num == 10.0);
	TEST(binder.get("ACTOR_01.attributes.sings").boolean == true);
	TEST(binder.get("player.name").str == "Test Player");

	// Save to a file for debugging
	std::string path = SavePath("test", "testluacore");
	std::ofstream stream = OpenSaveStream(path);
	zoneDriver.save(stream);
	stream.close();

	std::ostringstream buffer;
	zoneDriver.save(buffer);
	std::string str = buffer.str();

	TEST(str.find("STR") != std::string::npos);
	TEST(str.find("DEX") == std::string::npos);
}

void TestCombatant()
{
	{
		SWCombatant c;
		c.autoLevel(8, 4, 2, 1);
		TEST(c.fighting.d > 4);
		TEST(c.fighting.d >= c.shooting.d);
		TEST(c.shooting.d >= c.arcane.d);
	}
	{
		SWCombatant c;
		c.autoLevel(8, 4, 2, 0);
		TEST(c.fighting.d > 4);
		TEST(c.fighting.d >= c.shooting.d);
		TEST(c.arcane.d == 4 && c.arcane.b == -2);
	}
	{
		SWCombatant c;
		c.autoLevel(8, 2, 4, 0);
		TEST(c.shooting.d > 4);
		TEST(c.shooting.d >= c.fighting.d);
		TEST(c.arcane.d == 4 && c.arcane.b == -2);
	}
	{
		SWCombatant c;
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

		SWCombatant a;
		SWCombatant b;
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
	static void Read();
	static void TestSystem();
	static void TestScript(const ConstScriptAssets& ca, ScriptBridge& bridge);
};

void BattleTest::Read()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("");
	ScriptAssets assets(csa);
	CoreData coreData;
	VarBinder binder(bridge, coreData, ScriptEnv());

	const Battle& battle = assets.getBattle("TEST_BATTLE_1_CURSED_CAVERN");
	TEST(battle.name == "Cursed Cavern");
	TEST(battle.regions.size() == 4);
	TEST(battle.regions[1].name == "Shallow Pool");
	TEST(battle.regions[1].yards == 8);

	TEST(battle.regions[0].cover == Cover::kLightCover);

	TEST(battle.combatants.size() == 3);
	{
		const Combatant& c = assets.getCombatant(battle.combatants[1]);
		TEST(c.name == "Skeleton Archer");
		TEST(c.count == 1);
		TEST(c.fighting == 4);
		TEST(c.shooting == 6);
		TEST(c.arcane == 0);

		const Item& shortsword = assets.getItem("SHORTSWORD");
		const Item& bow = assets.getItem("BOW");

		TEST(shortsword.isMeleeWeapon());
		TEST(!shortsword.isRangedWeapon());
		TEST(!shortsword.isArmor());
		TEST(shortsword.range == 0);
		TEST(shortsword.damage == Die(1, 6, 0));

		TEST(bow.isRangedWeapon());
		TEST(!bow.isMeleeWeapon());
		TEST(!bow.isArmor());
		TEST(bow.range == 24);
		TEST(bow.damage == Die(1, 6, 0));

		TEST(c.inventory.numItems(shortsword) == 1);
		TEST(c.inventory.numItems(bow) == 1);
		TEST(c.inventory.meleeWeapon() == &shortsword);
		TEST(c.inventory.rangedWeapon() == &bow);
	}
	{
		const Combatant& c = assets.getCombatant(battle.combatants[0]);
		TEST(c.name == "Skeleton Warrior");
		TEST(c.count == 2);
		TEST(c.fighting == 6);
		TEST(c.shooting == 0);
		TEST(c.arcane == 0);
		TEST(c.bias == -1);

		const Item* pArmor = c.inventory.armor();
		TEST(pArmor != nullptr);
		TEST(pArmor->armor == 3);
		TEST(pArmor->isArmor());
	}
	{
		const Combatant& c = assets.getCombatant(battle.combatants[2]);
		TEST(c.name == "Skeleton Mage");
		TEST(c.count == 1);
		TEST(c.fighting == 0);
		TEST(c.shooting == 0);
		TEST(c.arcane == 6);

		const Power& p = assets.getPower(c.powers[0]);
		TEST(p.name == "Fire Bolt");
		TEST(p.effect == "bolt");
		TEST(p.cost == 1);
		TEST(p.range == 2);
		TEST(p.strength == 1);

		SWPower sp = SWPower::convert(p);
		TEST(sp.type == ModType::kBolt);
		TEST(sp.name == "Fire Bolt");
		TEST(sp.cost == 1);
		TEST(sp.rangeMult == 2);
		TEST(sp.effectMult == 1);
	}
	{
		const Actor& player = assets.getActor("testplayer");
		SWCombatant c = SWCombatant::convert(player, assets, binder);
		TEST(c.link == "testplayer");
		TEST(c.name == "Test Player");
		TEST(c.wild);
		TEST(c.fighting.d == 4);
		TEST(c.shooting.d == 8);
		TEST(c.arcane.d == 4);
		TEST(c.powers.size() == 1);
	}
}

void BattleTest::TestSystem()
{
	using namespace lurp::swbattle;

	Random random(100);
	BattleSystem system(random);

	system.setBattlefield("Landing Pad");
	system.addRegion({ "Mooring", 0, Cover::kLightCover });
	system.addRegion({ "Catwalk", 10, Cover::kNoCover });
	system.addRegion({ "Dock", 20, Cover::kMediumCover });

	RangedWeapon blaster = { "blaster", {2, 6, 0}, 2, 30 };
	MeleeWeapon baton = { "baton", {1, 4, 0} };
	Armor riotGear = { "riot gear", 2 };
	SWPower starCharm = { ModType::kBoost, "StarCharm", 3, 1, 1 };
	SWPower farCharm = { ModType::kBoost, "FarCharm", 3, 4, 1 };
	SWPower swol = { ModType::kBoost, "Swol", 1, 1, 1 };
	SWPower spark = { ModType::kBolt, "Spark", 1, 1, 1 };

	SWCombatant a;
	a.name = "Rogue Rebel";
	a.wild = true;
	a.autoLevel(8, 2, 4, 2, 0);
	a.rangedWeapon = blaster;
	a.powers.push_back(starCharm);
	a.powers.push_back(farCharm);

	SWCombatant b;
	b.name = "Brute";
	b.autoLevel(6, 4, 2, 0, 0);
	b.meleeWeapon = baton;	
	b.armor = riotGear;

	SWCombatant c;
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
	Action action = system.queue.pop();
	TEST(action.type == Action::Type::kAttack);
	const AttackAction& aa = std::get<AttackAction>(action.data);
	TEST(aa.attacker == 0);
	TEST(aa.defender == 1);
	TEST(aa.attackRoll.die == Die(1, 6, -6));
	TEST(aa.mods.size() == 2);
	TEST(aa.damageMods.size() == 0);
	TEST(aa.success == false);

}

void BattleTest::TestScript(const ConstScriptAssets& ca, ScriptBridge& bridge)
{
	ScriptAssets assets(ca);
	//MapData coreData(MapData::kSeed);
	//ScriptEnv env = { "TEST_BATTLE_1", NO_ENTITY, NO_ENTITY, "testplayer", NO_ENTITY };
	//ScriptDriver driver(assets, env, coreData, bridge);
	ZoneDriver zoneDriver(assets, bridge, "testplayer");
	ScriptDriver driver(zoneDriver, bridge, "TEST_BATTLE_1");

	TEST(driver.type() == ScriptType::kBattle);
	{
		// FIXME: driver.helper()->binder() is not clear
		BattleSystem battle(assets, driver.helper()->varBinder(), driver.battle(), "testplayer", zoneDriver.mapData.random);
		TEST(battle.name() == "Cursed Cavern");
		TEST(battle.combatants().size() == 5);
		TEST(battle.regions().size() == 4);

		battle.start(false);	// for testing, don't randomize the turn order
		TEST(battle.turnOrder().size() == 5);
		TEST(battle.turn() == 0);
		TEST(battle.playerTurn());

		TEST(battle.checkAttack(0, 1) == BattleSystem::ActionResult::kSuccess);
		TEST(battle.attack(0, 1) == BattleSystem::ActionResult::kSuccess);

		TEST(battle.queue.size() == 1);	// failure (if random/algo doesn't change)
		battle.queue.pop();

		battle.advance();

		TEST(battle.turn() == 1);
		TEST(!battle.playerTurn());
		battle.doEnemyActions();
		TEST(battle.queue.size() > 0);

		while (!battle.queue.empty())
			battle.queue.pop();

		TEST(!battle.done());
	}
}

int RunTests()
{
	ScriptBridge bridge;
	ConstScriptAssets csassets = bridge.readCSA("");

	RUN_TEST(BridgeWorkingTest(bridge));
	RUN_TEST(HaveCAssetsTest(csassets));
	RUN_TEST(BasicTest(csassets, bridge));
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
	RUN_TEST(TestSave(csassets, bridge));
	RUN_TEST(TestLoad());
	RUN_TEST(TestInventoryScript(csassets, bridge));
	RUN_TEST(TestWalkabout(csassets, bridge));
	RUN_TEST(TestLuaCore());
	RUN_TEST(TestContainers(csassets, bridge));
	RUN_TEST(TestCombatant());
	RUN_TEST(BattleTest::Read());
	RUN_TEST(BattleTest::TestSystem());
	RUN_TEST(BattleTest::TestScript(csassets, bridge));

	assert(gNTestPass > 0);
	assert(gNTestFail == 0);
	return TestReturnCode();
}

int TestReturnCode()
{
	if (gNTestPass == 0) return -1;
	return gNTestFail;
}
