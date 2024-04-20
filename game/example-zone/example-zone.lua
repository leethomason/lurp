--[[
    A a very simple Zone. Two rooms to move between.
    You can get a a key to open a locked door.
    And an npc to say "hello" to.
]]--

Actor {
    entityID = "player",
    name = "Finnian"
}

Actor {
    entityID = "Rando",
    name = "Rando"
}

Item {
    entityID = "KEY",
    name = "Key"
}

Zone {
    entityID = "ZONE",
    name = "Example Building",

    Room {
        entityID = "FOYER",
        name = "Foyer",
        Container {
            name = "Key Hook",
            items = { "KEY" }
        }
    },
    Room {
        entityID = "MAIN_HALL",
        name = "Main Hall",
        Interaction {
            entityID = "RANDO_SAYS_HI",     -- unique name of the interaction. can't be 'Rando' because that's the Actor name
            name = "Rando",                 -- displayed name of the interaction
            npc = "Rando",                  -- the entityID of the Actor to interact with
            next = Text {
                s="{npc.name}", "Hello there!"  -- and here we look up the name just in case we change Rando's name
            }
        }
    },
    Edge {
        name = "Door",
        room1 = "FOYER",
        room2 = "MAIN_HALL",
        locked = true,
        key = "KEY"
    }
}