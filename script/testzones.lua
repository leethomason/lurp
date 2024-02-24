
Item {
    entityID = "KEY_01",
    name = "Key-One"
}

Item {
    entityID = "GOLD",
    name = "Gold coins"
}

Actor {
    entityID = "TEST_ACTOR_1",
    name = "TestActor1",
}

Zone {
    entityID = "TEST_ZONE_0",
    name = "TestDungeon",
    Room {
        entityID = "TEST_ZONE_0_ROOM_A",
        name = "RoomA",
        desc = "Simple room.",
        Container {
            entityID = "TEST_ZONE_0_CHEST_01",
            name = "Chest",
            locked = false,
            items = { "KEY_01", { "GOLD", 10 } },
        },
    },
    Room {
        entityID = "TEST_ZONE_0_ROOM_B",
        name = "RoomB",
        Interaction { name = "Bookcase", next = "DIALOG_BOOKCASE_V1" },
        Interaction { name = "Move Button", next = Script {
            Text { "You push the button.", 
                    code = function() MovePlayer("TEST_ZONE_0_ROOM_A", false) end },
        }},
        Interaction { name = "Teleport Button", next = Script {
            Text { "You push the button.", 
                    code = function() MovePlayer("TEST_ROOM_1", true) end },
        }}
    },
    Edge {
        name = "Door",
        dir = "e",
        room1 = "TEST_ZONE_0_ROOM_A",
        room2 = "TEST_ZONE_0_ROOM_B",
        locked = true,
        key = "KEY_01"
    },
}

Zone {
    entityID = "TEST_ZONE_1",
    name = "Cyber",
    Room {
        entityID = "TEST_ROOM_1",
        name = "test room",
        Interaction {
            actor = "TEST_ACTOR_1",
            next = Script {
                entityID = "TEST_SCRIPT_1",
                code = function()
                    script.npcName = npc.name
                    script.scriptFlag = true
                    player.playerflag = true
                    npc.npcFlag = true
                    zone.zoneFlag = true
                    room.roomFlag = true
                end,
                Text {
                    s="narrator", "This is a test script in the '{zone.name}' zone's '{room.name}'."
                }
            }
        }
    }
}

Zone {
    entityID = "TEST_ZONE_2",
    name = "TestZone2",
    Room {
        entityID = "TEST_ROOM_2",
        name = "TestRoom2",

        Container {
            name = "CHEST_02_A",
            items = {{ "GOLD", 100 }},
            locked = true,
            key = "KEY_01"
        },

        Container {
            name = "CHEST_02_B",
            items = { "KEY_01"},
        }
    }
}