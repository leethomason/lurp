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

static int gNTestPass = 0;
static int gNTestFail = 0;
static int gNRunTest = 0;

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

void BridgeWorkingTest()
{
	ScriptBridge bridge;
	TEST(bridge.getLuaState());
}

void HaveCAssetsTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testzones.lua");

	TEST(ca.zones.size() > 0);
	TEST(ca.scripts.size() > 0);

	ScriptAssets assets(ca);
	TEST(assets.getZone("TEST_ZONE_0").name == "TestDungeon");
}

static void BasicTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testzones.lua");

	ScriptAssets assets(ca);
	ZoneDriver map(assets, bridge, "TEST_ZONE_0");

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
	TEST(map.news().size() == 1);
	map.news().clear();

	// 2. open the chest and get the key
	ContainerVec containerVec = map.getContainers();
	TEST(containerVec.size() == 1);
	const Container& chest = *containerVec[0];
	Inventory& chestInv = assets.inventories.at(chest.entityID);

	const Item& key01 = assets.getItem("KEY_01");
	TEST(chestInv.hasItem(key01) == true);
	map.transfer(key01, chest.entityID, map.getPlayer().entityID, 1);
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


static void DialogTest_Bookcase(const EntityID& dialog)
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");

	ScriptAssets assets(ca);
	MapData mapData(56);
	ScriptEnv env;
	env.script = dialog;
	ScriptDriver dd(assets, mapData, bridge, env);
	VarBinder binder = dd.varBinder();

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
	Item book("ITEM_BOOK", "book");
	Item potion("ITEM_POTION", "potion");

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
	Item potion("ITEM_POTION", "potion");

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
	ConstScriptAssets csa = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(csa);

	ScriptEnv env;
	MapData mapData(56);
	ScriptDriver driver(assets, mapData, bridge, env);
	VarBinder binder = driver.varBinder();

	ScriptRef ref = assets.getScriptRef("player");
	TEST(ref.type == ScriptType::kActor);
	const Actor& player = assets._csa.actors[ref.index];
	TEST(player.name == "Test Player");

	TEST(binder.get("player.fighting").num == 4.0);
	binder.set("player.fighting", 5.0);
	TEST(binder.get("player.fighting").num == 5.0);
	binder.set("player.fighting", 4.0);
	TEST(binder.get("player.fighting").num == 4.0);
}

static void TestScriptSave()
{
	std::filesystem::path path = SavePath("test", "testsave");

	for (int story = 0; story < 3; story++) {

		// SAVE
		{
			ScriptBridge bridge;
			ConstScriptAssets csa = bridge.readCSA("game/example-script/example-script.lua");
			ScriptAssets assets(csa);
			MapData data(56);
			ScriptEnv env;
			env.script = "CASINO_SCRIPT";
			ScriptDriver driver(assets, data, bridge, env);

			if (story >= 0) {
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "As the elevator descends from the top floors of the Zephyr Hotel, you adjust your...");
			}
			if (story >= 1) {
				driver.advance();
				TEST(driver.type() == ScriptType::kChoices);
				driver.choose(1);	// dress
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "...in the mirror and double-check the semi-auto inside your purse.");
			}
			if (story >= 2) {
				driver.advance();
				driver.advance();
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "Waitress, I'll have a...");
				driver.advance();
				TEST(driver.type() == ScriptType::kChoices);
			}

			std::ofstream stream = OpenSaveStream(path);
			data.coreData.save(stream);	// so filtering will work
			driver.save(stream);
		}

		// LOAD
		{
			ScriptBridge bridge;
			ConstScriptAssets csa = bridge.readCSA("game/example-script/example-script.lua");
			ScriptAssets assets(csa);
			MapData data(48);	// note the change of the random number gen
			ScriptEnv env;
			env.script = "CASINO_SCRIPT";
			ScriptDriver driver(assets, data, bridge, env);

			ScriptBridge loader;
			loader.loadLUA(path.string());
			data.coreData.load(loader);	 // needed for filtering - must be before the driver.load()
			bool success = driver.load(loader);
			TEST(success);

			if (story == 0) {
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "As the elevator descends from the top floors of the Zephyr Hotel, you adjust your...");
			}
			else if (story == 1) {
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "...in the mirror and double-check the semi-auto inside your purse.")
			}
			else if (story == 2) {
				TEST(driver.type() == ScriptType::kChoices);
			}
		}
	}
}

static void TestScriptSaveMutated()
{
	std::filesystem::path path = SavePath("test", "testsave");

	for (int story = 0; story < 3; story++) {

		// SAVE
		{
			ScriptBridge bridge;
			ConstScriptAssets csa = bridge.readCSA("game/example-script/example-script.lua");
			ScriptAssets assets(csa);
			MapData data(56);
			ScriptEnv env;
			env.script = "CASINO_SCRIPT";
			ScriptDriver driver(assets, data, bridge, env);

			if (story >= 0) {
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "As the elevator descends from the top floors of the Zephyr Hotel, you adjust your...");
			}
			if (story >= 1) {
				driver.advance();
				TEST(driver.type() == ScriptType::kChoices);
				driver.choose(1);	// dress
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "...in the mirror and double-check the semi-auto inside your purse.");
			}
			if (story >= 2) {
				driver.advance();
				driver.advance();
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "Waitress, I'll have a...");
				driver.advance();
				TEST(driver.type() == ScriptType::kChoices);
			}

			std::ofstream stream = OpenSaveStream(path);
			data.coreData.save(stream);	// so filtering will work
			driver.save(stream);
		}

		// LOAD
		{
			ScriptBridge bridge;
			bridge.setGlobal("_STARTING_POOL_ID", 123);
			ConstScriptAssets csa = bridge.readCSA("game/example-script/example-script.lua");
			ScriptAssets assets(csa);
			MapData data(48);	// note the change of the random number gen
			ScriptEnv env;
			env.script = "CASINO_SCRIPT";
			ScriptDriver driver(assets, data, bridge, env);

			ScriptBridge loader;
			loader.loadLUA(path.string());
			data.coreData.load(loader);	 // needed for filtering - must be before the driver.load()
			driver.load(loader);

			if (story == 0) {
				// This SHOULD work because this is the first thing in the Script - a rollback will land us here.
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "As the elevator descends from the top floors of the Zephyr Hotel, you adjust your...");
			}
			else if (story == 1) {
				// This SHOULD work because ENTERING_CASINO_TEXT anchors the script - even though the IDs are mutated, this is stable.
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "...in the mirror and double-check the semi-auto inside your purse.")
			}
			else if (story == 2) {
				// This will not work because Choices doesn't have an ID, and it has to roll back.
				// TEST(driver.type() == ScriptType::kChoices);
				// Here's the roll-back:
				TEST(driver.type() == ScriptType::kText);
				TEST(driver.line().text == "As the elevator descends from the top floors of the Zephyr Hotel, you adjust your...");
			}
		}
	}
}

static void TestZoneSave()
{
	std::filesystem::path path = SavePath("test", "testsave");

	for (int story = 0; story < 3; story++) {

		// SAVE
		{
			ScriptBridge bridge;
			ConstScriptAssets csa = bridge.readCSA("game/example-zone/example-zone.lua");
			ScriptAssets assets(csa);
			ZoneDriver zd(assets, bridge, "ZONE");

			if (story >= 0) {
				TEST(zd.currentRoom().entityID == "FOYER");
				TEST(zd.mode() == ZoneDriver::Mode::kNavigation);
				TEST(zd.getContainers().size() == 1);
				const Container* c = zd.getContainers()[0];
				TEST(c);
				zd.transferAll(c->entityID, zd.getPlayer().entityID);
				const Inventory& inv = assets.getInventory(*c);
				TEST(inv.emtpy());
			}
			if (story >= 1) {
				TEST(zd.move("MAIN_HALL") == ZoneDriver::MoveResult::kSuccess);
				TEST(zd.getInteractions().size() == 1);
				zd.startInteraction(zd.getInteractions()[0]);
				TEST(zd.mode() == ZoneDriver::Mode::kText);
				TEST(zd.text().text == "Hello there!");
			}
			if (story >=2) {
				zd.advance();
				TEST(zd.mode() == ZoneDriver::Mode::kNavigation);
			}

			std::ofstream stream = OpenSaveStream(path);
			zd.save(stream);
		}
		// LOAD
		{
			ScriptBridge bridge;
			ConstScriptAssets csa = bridge.readCSA("game/example-zone/example-zone.lua");
			ScriptAssets assets(csa);
			ZoneDriver zd(assets, bridge, "ZONE");

			ScriptBridge loader;
			loader.loadLUA(path.string());
			zd.load(loader);

			if (story == 0) {
				TEST(zd.currentRoom().entityID == "FOYER");
				TEST(zd.mode() == ZoneDriver::Mode::kNavigation);
				TEST(zd.getContainers().size() == 1);
				const Container* c = zd.getContainers()[0];
				TEST(c);
				const Inventory& inv = assets.getInventory(*c);
				TEST(inv.emtpy());
			}
			else if (story == 1) {
				TEST(zd.currentRoom().entityID == "MAIN_HALL");
				TEST(zd.mode() == ZoneDriver::Mode::kText);
				TEST(zd.text().text == "Hello there!");
			}
			else if (story == 2) {
				TEST(zd.currentRoom().entityID == "MAIN_HALL");
				TEST(zd.mode() == ZoneDriver::Mode::kNavigation);
			}
		}
	}
}

static void TestCodeEval()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(csa);
	MapData mapData(56);
	ScriptEnv env = { "TEST_MAGIC_BOOK", NO_ENTITY, NO_ENTITY, NO_ENTITY };
	
	{
		{
			VarBinder binder(assets, bridge, mapData.coreData, env);
			binder.set("player.class", Variant("fighter"));
			binder.set("player.arcaneGlow", Variant());
			binder.set("player.mystery", Variant());
		}
		ScriptEnv env2 = env;
		env2.script = "TEST_MAGIC_BOOK";
		ScriptDriver driver(assets, mapData, bridge, env2);
		TEST(driver.type() == ScriptType::kChoices);
		TEST(driver.choices().choices.size() == 1);
		driver.choose(0);
		TEST(driver.done());
	}
	{
		{
			VarBinder binder(assets, bridge, mapData.coreData, env);
			binder.set("player.class", Variant("druid"));
			binder.set("player.arcaneGlow", Variant());
			binder.set("player.mystery", Variant());
		}

		ScriptEnv env2 = env;
		env.script = "TEST_MAGIC_BOOK";
		ScriptDriver driver(assets, mapData, bridge, env2);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}
	{
		{
			VarBinder binder(assets, bridge, mapData.coreData, env);
			// Set up a run
			binder.set("player.class", Variant("wizard"));
			binder.set("player.arcaneGlow", Variant());
			binder.set("player.mystery", Variant());
		}

		ScriptEnv env2 = env;
		env.script = "TEST_MAGIC_BOOK";
		ScriptDriver driver(assets, mapData, bridge, env2);
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

static void TestScriptBridge()
{
	ScriptBridge engine;

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

static void TestTextSubstitution()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	env.script = "_TEST_READING";
	MapData mapData(56);
	ScriptDriver driver(assets, mapData, bridge, env);

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

static void TestTextTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	MapData mapData(56);
	env.script = "_TEST_TEXT_TEST";
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "This is pathA");
	driver.advance();
	TEST(driver.line().text == "This is NOT pathB");
	driver.advance();
	TEST(driver.done());
}

static void CallScriptTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	MapData mapData(56);
	env.script = "_OPEN_RED_CHEST";
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().speaker == "narrator");
	TEST(driver.line().text == "You see a chest with a red gem in the center.");
	driver.advance();
	TEST(driver.done());
}

static void ChoiceMode1RepeatTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	MapData mapData(56);
	env.script = "CHOICE_MODE_1_REPEAT";
	ScriptDriver driver(assets, mapData, bridge, env);

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

static void ChoiceMode1RewindTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	MapData mapData(56);
	env.script = "CHOICE_MODE_1_REWIND";
	ScriptDriver driver(assets, mapData, bridge, env);

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

static void ChoiceMode1PopTest()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	MapData mapData(56);
	env.script = "CHOICE_MODE_1_POP";
	ScriptDriver driver(assets, mapData, bridge, env);

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

static void ChoiceMode2Test()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ScriptEnv env;
	MapData mapData(56);
	env.script = "CHOICE_MODE_2_TEST";
	ScriptDriver driver(assets, mapData, bridge, env);

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

static void FlagTest()
{
	using Mode = ZoneDriver::Mode;

	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testzones.lua");
	ScriptAssets assets(ca);
	ZoneDriver driver(assets, bridge, "TEST_ZONE_1");

	TEST(driver.mode() == Mode::kText);
	TEST(driver.mapData.coreData.coreGet("TEST_ACTOR_1", "npcFlag").second.boolean == true);
	TEST(driver.mapData.coreData.coreGet("_ScriptEnv", "scriptFlag").second.boolean == true);
	TEST(driver.mapData.coreData.coreGet("_ScriptEnv", "npcName").second.str == "TestActor1");
	driver.advance();
	TEST(driver.mode() == Mode::kNavigation);

	TEST(driver.mapData.coreData.coreGet("TEST_ACTOR_1", "npcFlag").second.boolean == true);
	TEST(driver.mapData.coreData.coreGet("_ScriptEnv", "scriptFlag").second.type == LUA_TNIL);
}


#if 0 // fixme: needs to work on a script & zone example
static void TestInventoryScript()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	ZoneDriver map(assets, bridge);
	ScriptEnv env = map.getScriptEnv();
	env.script = "KEY_MASTER";
	ScriptDriver driver(assets, map.mapData, bridge, env);

	TEST(driver.type() == ScriptType::kChoices);
	TEST(driver.choices().choices.size() == 2);
	TEST(driver.choices().choices[0].text == "Get the key");
	driver.choose(0);
	TEST(driver.type() == ScriptType::kText);
	TEST(driver.line().text == "You have the Skeleton Key.");
	driver.advance();
	TEST(driver.done());

	TEST(map.numItems("player", "SKELETON_KEY") == 1);
}
#endif 

static void TestContainers()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testzones.lua");
	ScriptAssets assets(ca);
	ZoneDriver zone(assets, bridge, "TEST_ZONE_2");
	zone.setZone("TEST_ZONE_2", "TEST_ROOM_2");

	const Actor& player = zone.getPlayer();

	TEST(zone.currentRoom().name == "TestRoom2");
	TEST(zone.getContainers().size() == 2);

	// 1. Makes sure we can't open the locked chest.
	ContainerVec containerVec = zone.getContainers();
	const Container& chestA = *containerVec[0];
	const Container& chestB = *containerVec[1];

	TEST(zone.transferAll(chestA.entityID, player.entityID) == ZoneDriver::TransferResult::kLocked);
	TEST(zone.news().size() == 1);
	zone.news().clear();

	TEST(zone.transferAll(chestB.entityID, player.entityID) == ZoneDriver::TransferResult::kSuccess);
	TEST(zone.mapData.newsQueue.size() == 1); // got a key
	zone.mapData.newsQueue.clear();

	TEST(zone.transferAll(chestA.entityID, player.entityID) == ZoneDriver::TransferResult::kSuccess);
	TEST(zone.getInventory(player).numItems(assets.getItem("GOLD")) == 100);
	TEST(zone.mapData.newsQueue.size() == 2);	// unlocked and have gold
}

static void TestWalkabout()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testzones.lua");
	ScriptAssets assets(ca);
	ZoneDriver zone(assets, bridge, "TEST_ZONE_0");
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
		InteractionVec interactions = zone.getInteractions();
		TEST(interactions.size() == 2);
		const Interaction* iAct = interactions[0];
		TEST(iAct->name == "Move Button");
		ScriptEnv env = zone.getScriptEnv(iAct);
		ScriptDriver driver(assets, zone.mapData, bridge, env);
		TEST(driver.type() == ScriptType::kText);
		driver.advance();
		TEST(driver.done());
	}
	TEST(zone.currentRoom().entityID == "TEST_ZONE_0_ROOM_A");
	zone.move("TEST_ZONE_0_ROOM_B");

	// Push the teleport button:
	{
		InteractionVec interactions = zone.getInteractions();
		TEST(interactions.size() == 2);
		const Interaction* iAct = interactions[1];
		TEST(iAct->name == "Teleport Button");
		ScriptEnv env = zone.getScriptEnv(iAct);
		ScriptDriver driver(assets, zone.mapData, bridge, env);
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
	ConstScriptAssets csa = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(csa);
	ScriptEnv env;
	env.script = "TEST_LUA_CORE";
	MapData mapData(56);
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kText);
	CoreData& cd = mapData.coreData;
	TEST(cd.coreGet("ACTOR_01", "STR").second.num == 17.0);
	TEST(cd.coreGet("ACTOR_01", "attributes").first == false);
	cd.coreSet("ACTOR_01", "STR", Variant(18.0), false);

	driver.advance();
	TEST(driver.done());
	TEST(cd.coreGet("ACTOR_01", "STR").second.num == 18.0);

	// Test the path works the same as the Core
	VarBinder binder = driver.varBinder();
	TEST(binder.get("ACTOR_01.STR").num == 18.0);
	TEST(binder.get("ACTOR_01.DEX").num == 10.0);
	TEST(binder.get("ACTOR_01.attributes.sings").boolean == true);
	TEST(binder.get("player.name").str == "Test Player");

	// Make sure we can't set a read-only value
	binder.set("player.name", "Foozle");
	TEST(binder.get("player.name").str == "Test Player");

	// At this point we can't save the 'driver' because
	// it is done(). But we can test save/load of the CoreData

	// Save to a file for debugging
	std::filesystem::path path = SavePath("test", "testluacore");
	std::ofstream stream = OpenSaveStream(path);
	cd.save(stream);
	stream.close();

	// Do the same on a stream to test
	std::ostringstream buffer;
	cd.save(buffer);
	std::string str = buffer.str();

	TEST(str.find("STR") != std::string::npos);
	TEST(str.find("DEX") == std::string::npos);
}

void TestCombatant()
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

class BattleTest {
public:
	static void Read();
	static void TestSystem();
	static void TestScript();
	static void TestExample();
	static void TestRegionSpells();
	static void TestBattle2();
};

void BattleTest::Read()
{
	ScriptBridge bridge;
	ConstScriptAssets csa = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(csa);
	CoreData coreData;
	VarBinder binder(assets, bridge, coreData, ScriptEnv());

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
		const Actor& player = assets.getActor("player");
		SWCombatant c = SWCombatant::convert(player, assets, binder);
		TEST(c.link == "player");
		TEST(c.name == "Test Player");
		TEST(c.wild);
		TEST(c.fighting.d == 4);
		TEST(c.shooting.d == 8);
		TEST(c.arcane.d == 4);
		TEST(c.powers.size() == 1);
	}
}

void BattleTest::TestRegionSpells()
{
	using namespace lurp::swbattle;

	Random random(100);
	BattleSystem system(random);

	system.setBattlefield("Landing Pad");
	system.addRegion({ "Mooring", 0, Cover::kLightCover });
	system.addRegion({ "Catwalk", 10, Cover::kNoCover });
	system.addRegion({ "Dock", 20, Cover::kMediumCover });

	SWPower arcaneStorm = { ModType::kBolt, "ArcaneStorm", 1, 4, 1, true };

	SWCombatant a;
	a.name = "StarBlaster";
	a.wild = true;
	a.arcane = Die(1, 12, 0);
	a.powers.push_back(arcaneStorm);

	SWCombatant b;
	b.name = "Brute B";
	b.fighting = Die(1, 6, 0);

	SWCombatant c;
	b.name = "Brute C";
	b.fighting = Die(1, 6, 0);

	system.addCombatant(a);
	system.addCombatant(b);
	system.addCombatant(c);

	system.start(false);

	TEST(system.queue.empty());
	TEST(system.turn() == 0);
	TEST(system.checkPower(0, 1, 0) == BattleSystem::ActionResult::kSuccess);
	system.power(0, 1, 0);
	TEST(system.queue.size() == 2);	// possible this will fail if the PRNG changes
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

	// cost, range, effect
	SWPower starCharm = { ModType::kCombatBuff, "StarCharm", 3, 1, 1 };
	SWPower farCharm = { ModType::kCombatBuff, "FarCharm", 3, 4, 1 };
	SWPower swol = { ModType::kMeleeBuff, "Swol", 1, 1, 1 };
	SWPower spark = { ModType::kBolt, "Spark", 1, 1, 1 };

	SWCombatant a;
	a.name = "Rogue Rebel";
	a.wild = true;
	a.fighting = Die(1, 6, 0);
	a.shooting = Die(1, 8, 0);
	a.arcane = Die(1, 6, 0);
	a.rangedWeapon = blaster;
	a.powers.push_back(starCharm);
	a.powers.push_back(farCharm);

	SWCombatant b;
	b.name = "Brute";
	b.fighting = Die(1, 6, 0);
	b.meleeWeapon = baton;	
	b.armor = riotGear;

	SWCombatant c;
	c.name = "Sparky";
	c.shooting = Die(1, 6, 0);
	c.arcane = Die(1, 6, 0);
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
	TEST(p.first == Die(1, 8, -6));
	TEST(p.second == 4);
	TEST(mods.size() == 2);
	TEST(mods[0].type == ModType::kRange);
	TEST(mods[1].type == ModType::kCover);
	mods.clear();

	system.place(0, 2);
	p = system.calcMelee(0, 1, mods);
	TEST(p.first == Die(1, 6, 0));
	TEST(p.second == 5);
	TEST(mods.size() == 0);

	system.place(0, 0);
	TEST(system.attack(0, 1) == BattleSystem::ActionResult::kSuccess);
	TEST(system.queue.size() == 1);
	Action action = system.queue.pop();
	TEST(action.type == Action::Type::kAttack);
	const AttackAction& aa = std::get<AttackAction>(action.data);
	TEST(aa.attacker == 0);
	TEST(aa.defender == 1);
	TEST(aa.attackRoll.die == Die(1, 8, -6));
	TEST(aa.mods.size() == 2);
	TEST(aa.success == false);

}

void BattleTest::TestScript()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	MapData mapData(56);
	ScriptEnv env;
	env.script = "TEST_BATTLE_1";
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kBattle);
	{
		// FIXME: driver.helper()->binder() is not clear
		BattleSystem battle(assets, driver.varBinder(), driver.battle(), "player", mapData.random);
		TEST(battle.name() == "Cursed Cavern");
		TEST(battle.combatants().size() == 5);
		TEST(battle.regions().size() == 4);

		battle.start(false);	// for testing, don't randomize the turn order
		TEST(battle.turnOrder().size() == 5);
		TEST(battle.turn() == 0);
		TEST(battle.playerTurn());

		TEST(battle.checkAttack(0, 1) == BattleSystem::ActionResult::kSuccess);
		TEST(battle.attack(0, 1) == BattleSystem::ActionResult::kSuccess);

		TEST(battle.queue.size() == 1);	// could fail if the PRNG changes
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

void BattleTest::TestExample()
{
	ScriptBridge bridge;
	ConstScriptAssets csassets = bridge.readCSA("game/example-battle/example-battle.lua");
	ScriptAssets assets(csassets);
	ZoneDriver driver(assets, bridge, "BATTLE_ZONE");

	TEST(driver.mode() == ZoneDriver::Mode::kText);	// "choose your gear"
	driver.advance();
	TEST(driver.mode() == ZoneDriver::Mode::kChoices);
	driver.choose(0);
	TEST(driver.mode() == ZoneDriver::Mode::kText);	// "to the arena!"
	driver.advance();
	TEST(driver.mode() == ZoneDriver::Mode::kChoices);
	driver.choose(0);
	TEST(driver.mode() == ZoneDriver::Mode::kBattle);
	driver.battleDone();

	// Well this is subtle. When does the 'code' property actually run?
	// `battleDone()` actually advances to the text w/ code
	// `mode()` doesn't have to be called
	// `text()` could get called more than once...
	// Okay. `text()` is the one you would expect, I think.

	TEST(driver.mode() == ZoneDriver::Mode::kText);	// "you win!"
	driver.text();									
	driver.advance();
	TEST(driver.isGameOver() == true);
}

void BattleTest::TestBattle2()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	MapData mapData(56);
	ScriptEnv env;
	env.script = "TEST_BATTLE_2";
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kBattle);

	BattleSystem battle(assets, driver.varBinder(), driver.battle(), "player", mapData.random);
	TEST(battle.combatants().size() == 4);
}

static void TestExampleZone()
{
	ScriptBridge bridge;
	ConstScriptAssets csassets = bridge.readCSA("game/example-zone/example-zone.lua");
	ScriptAssets assets(csassets);
	ZoneDriver driver(assets, bridge, "ZONE");

	TEST(driver.mode() == ZoneDriver::Mode::kNavigation);
	TEST(driver.getContainers().size() == 1);
	const Container* c = driver.getContainers()[0];
	TEST(c);
	driver.transferAll(c->entityID, driver.getPlayer().entityID);
	const Inventory& inv = assets.getInventory(*c);
	TEST(inv.emtpy());

	TEST(driver.move("MAIN_HALL") == ZoneDriver::MoveResult::kSuccess);	
}

std::pair<std::string, std::string> makeStrPair(const std::string& a, const std::string& b) {
	return std::make_pair(a, b);
}

static void TestTextParsing()
{
	{
		TEST(trimRight("aa") == "aa");
		TEST(trimRight("aa ") == "aa");
		TEST(trimRight("  ").empty());
		TEST(trimRight(" aa") == " aa");

		TEST(trimLeft("aa") == "aa");
		TEST(trimLeft(" aa") == "aa");
		TEST(trimLeft("  ").empty());
		TEST(trimLeft("aa ") == "aa ");

		TEST(trim("aa") == "aa");
		TEST(trim(" aa ") == "aa");
		TEST(trim("  ").empty());
	}

	{
		//                  0    5    0
		std::string test = "aaa{B=c, {D=e}}";
		size_t end = parseRegion(test, 3, '{', '}');
		TEST(end == test.size());
	}
	{
		std::string test = "aaa{B=c, {D=e}}  ";
		size_t end = parseRegion(test, 3, '{', '}');
		TEST(end == 15);
	}
	{
		std::string test = "A=1, B = {22},C=3 , D= \"4 4\", E='55 55' ";
		std::vector<std::string> kv = parseKVPairs(test);
		TEST(kv.size() == 5);
		TEST(kv[0] == "A=1");
		TEST(kv[1] == "B = {22}");
		TEST(kv[2] == "C=3");
		TEST(kv[3] == "D= \"4 4\"");
		TEST(kv[4] == "E='55 55'");
	}
	{
		TEST(parseKV("A=1") == makeStrPair("A", "1"));
		TEST(parseKV("B= {22}") == makeStrPair("B", "{22}"));
		TEST(parseKV("D = \"4 4\"") == makeStrPair("D", "4 4"));
		TEST(parseKV("E='55 55'") == makeStrPair("E", "55 55"));
	}
}

static void TestInlineText()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	ScriptAssets assets(ca);
	MapData mapData(56);
	ScriptEnv env;
	env.script = "ALT_TEXT_1";
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kText);

	std::string speaker = driver.line().speaker;
	std::string text = driver.line().text;
	TEST(speaker == "Talker");
	TEST(text == "I'm going to tell a story. It will be fun.");
	
	driver.advance();
	speaker = driver.line().speaker;
	text = driver.line().text;
	TEST(speaker == "Talker");
	TEST(text == "Listen closely!");

	driver.advance();
	TEST(driver.line().speaker == "Listener");
	TEST(driver.line().text == "Yay!");

	driver.advance();
	TEST(driver.line().speaker == "Another");
	TEST(driver.line().text == "I want to hear too!");

	driver.advance();
	TEST(driver.done());
}

static void TestFormatting()
{
	ScriptBridge bridge;
	ConstScriptAssets ca = bridge.readCSA("script/testscript.lua");
	// fixme: More to be done here; mostly now just testing it loads.
	ScriptAssets assets(ca);
	MapData mapData(56);
	ScriptEnv env;
	env.script = "ALT_TEXT_2";
	ScriptDriver driver(assets, mapData, bridge, env);

	TEST(driver.type() == ScriptType::kText);
}

static void FlushText(ZoneDriver& driver)
{
	while (driver.mode() == ZoneDriver::Mode::kText) {
		driver.text();		// this call sets the text read
		driver.advance();
	}
}

static void TestChullu()
{
	ScriptBridge bridge;
	ConstScriptAssets csassets = bridge.readCSA("game/chullu/chullu.lua");
	ScriptAssets assets(csassets);
	ZoneDriver driver(assets, bridge, NO_ENTITY);

	TEST(driver.mode() == ZoneDriver::Mode::kText);
	TEST(driver.currentRoom().name == "Your Apartment");
	FlushText(driver);

	ZoneDriver::MoveResult mr = driver.move("SF_CAFE");
	TEST(mr == ZoneDriver::MoveResult::kSuccess);
	TEST(driver.currentRoom().name == "The Black Soul Cafe");
	FlushText(driver);

	mr = driver.move("SF_LIBRARY");
	TEST(mr == ZoneDriver::MoveResult::kSuccess);
	TEST(driver.currentRoom().name == "The SF City Library");
	FlushText(driver);
	{
		const Inventory& inv = driver.getInventory(driver.getPlayer());
		TEST(inv.numItems(assets.getItem("HAIRPIN")) == 1);
	}

	InteractionVec interactions = driver.getInteractions();
	TEST(interactions.size() == 1);
	driver.startInteraction(interactions[0]);
	TEST(driver.mode() == ZoneDriver::Mode::kChoices);
	FlushText(driver);

	TEST(driver.mode() == ZoneDriver::Mode::kChoices);
	driver.choose(1);	// shoot the lock off!
	FlushText(driver);
	mr = driver.move("SF_LIBRARY_RESTRICTED");
	TEST(mr == ZoneDriver::MoveResult::kLocked);

	driver.startInteraction(interactions[0]);
	FlushText(driver);
	driver.choose(0);	// pick the lock
	FlushText(driver);

	mr = driver.move("SF_LIBRARY_RESTRICTED");
	TEST(mr == ZoneDriver::MoveResult::kSuccess);
	FlushText(driver);
	ContainerVec containers = driver.getContainers();
	TEST(containers.size() == 1);
	driver.transferAll(containers[0]->entityID, driver.getPlayer().entityID);

	mr = driver.move("SF_LIBRARY");
	TEST(mr == ZoneDriver::MoveResult::kSuccess);
	FlushText(driver);
	{
		const Inventory& inv = driver.getInventory(driver.getPlayer());
		TEST(inv.numItems(assets.getItem("HAIRPIN")) == 0);
	}
	mr = driver.move("SF_LIBRARY_RESTRICTED");
	TEST(mr == ZoneDriver::MoveResult::kLocked);
	FlushText(driver);

	mr = driver.move("SF_CAFE");
	TEST(mr == ZoneDriver::MoveResult::kSuccess);
	FlushText(driver);
	TEST(driver.choices().choices.size() == 3);
	driver.choose(0);
	FlushText(driver);
	TEST(driver.choices().choices.size() == 3);
	driver.choose(1);
	FlushText(driver);
	TEST(driver.choices().choices.size() == 3);
	driver.choose(2);
	FlushText(driver);
	TEST(driver.choices().choices.size() == 4);
	driver.choose(3);
	FlushText(driver);	// adventure awaits!
}

int RunTests()
{
	RUN_TEST(BridgeWorkingTest());
	RUN_TEST(HaveCAssetsTest());
	RUN_TEST(BasicTest());
	RUN_TEST(TestScriptBridge());
	RUN_TEST(DialogTest_Bookcase("DIALOG_BOOKCASE_V1"));
	RUN_TEST(DialogTest_Bookcase("DIALOG_BOOKCASE_V2"));
	RUN_TEST(TestInventory1());
	RUN_TEST(TestInventory2());
	RUN_TEST(TestScriptAccess());
	RUN_TEST(TestCodeEval());
	RUN_TEST(TestBattle());
	RUN_TEST(TestTextSubstitution());
	RUN_TEST(TestTextTest());
	RUN_TEST(CallScriptTest());
	RUN_TEST(ChoiceMode1RepeatTest());
	RUN_TEST(ChoiceMode1RewindTest());
	RUN_TEST(ChoiceMode1PopTest());
	RUN_TEST(ChoiceMode2Test());
	RUN_TEST(FlagTest());
	RUN_TEST(TestScriptSave());
	RUN_TEST(TestScriptSaveMutated());
	RUN_TEST(TestZoneSave());
	RUN_TEST(TestWalkabout());
	RUN_TEST(TestLuaCore());
	RUN_TEST(TestContainers());
	RUN_TEST(TestCombatant());
	RUN_TEST(BattleTest::Read());
	RUN_TEST(BattleTest::TestSystem());
	RUN_TEST(BattleTest::TestScript());
	RUN_TEST(BattleTest::TestExample());
	RUN_TEST(BattleTest::TestRegionSpells());
	RUN_TEST(BattleTest::TestBattle2());
	RUN_TEST(TestExampleZone());
	RUN_TEST(TestTextParsing());
	RUN_TEST(TestInlineText());
	RUN_TEST(TestFormatting());
	RUN_TEST(TestChullu());

	assert(gNTestPass > 0);
	assert(gNTestFail == 0);
	return TestReturnCode();
}

int TestReturnCode()
{
	if (gNTestPass == 0) return -1;
	return gNTestFail;
}
