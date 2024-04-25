Item {
    entityID = "LONGSWORD",
    name = "longsword",
    desc = "A basic but very effective sword.",
    range = 0,
    damage = "d8",
}

Item {
    entityID = "SHORTSWORD",
    name = "shortsword",
    desc = "A short sword for close combat.",
    range = 0,
    damage = "d6",
}

Item {
    entityID = "BOW",
    name = "bow",
    range = 24,
    damage = "d6",
}

Item {
    entityID = "CHAINMAIL",
    name = "chainmail",
    armor = 3,
}

Power {
    entityID = "FIRE_BOLT",
    name = "Fire Bolt",
    effect = "bolt",
    cost = 1,
    range = 2,
    strength = 1,
}

Actor {
    entityID = "testplayer",
    name = "Test Player",
    wild = true,

    fighting = 4,
    shooting = 8,
    arcane = 4,

    items = { "LONGSWORD", "BOW", "CHAINMAIL"},
    powers = { "FIRE_BOLT" },
}

Script {
    entityID = "DIALOG_BOOKCASE_V1",
    Text {
        { s="narrator", "You see a bookcase.",
                        "The books are in a strange language." },
    },
    Choices {
        { text = "Read a book", next = "DIALOG_BOOKCASE_READ" },
        { text = "Leave", next = "done" },
        { text = "Portal", next = "pop" }
    },
    Text{
        eval = function()
            return player.arcaneGlow
        end,
        s="narrator", "You have an arcane glow.",
    },
    Text {
        s="narrator", "You step away from the bookcase.",
    }
}

Script {
    entityID = "DIALOG_BOOKCASE_READ",
    Text {
        { s="narrator", "You read a book." },
    },
    Choices {
        { text = "Read another book", next = "rewind" },
        { text = "Read the arcane book", next = "done", code = function()
            player.arcaneGlow = true
        end },
        { text = "Repeat the choices...", next = "repeat"},
        { text = "Leave", next = "pop" },
    },
}

Script {
    entityID = "DIALOG_BOOKCASE_V2",
    Text {
        { s="narrator", "You see a bookcase.",
                      "The books are in a strange language." },
    },
    Choices {
        { text = "Read a book", next =
            Script {
                Text {
                    { s="narrator", "You read a book." },
                },
                Choices {
                    { text = "Read another book", next = "rewind" },
                    { text = "Read the arcane book", next = "done", code = function()
                        player.arcaneGlow = true
                    end },
                    { text = "Repeat the choices...", next = "repeat"},
                    { text = "Leave", next = "pop" },
                },
            }
        },
        { text = "Leave", next = "done" },
        { text = "Portal", next = "pop"}
    },
    Text{
        eval = function()
            return player.arcaneGlow
        end,
        { s="narrator", "You have an arcane glow." },
    },
    Text {
        { s="narrator", "You step away from the bookcase." },
    }
}

Script {
    entityID = "TEST_MAGIC_BOOK",
    Text {
        eval = function ()
            assert(player._isCoreTable, "player is not a core table")
            return player.class == "druid"
        end,
        { s="narrator", "The book is magic, but not druid magic."}
    },
    Text {
        eval = function() return player.class == "wizard" end,
        { s="narrator", "Wizards know their books...but you don't know this one."},
        code = function() player.mystery = true end,
    },
    Choices {
        { eval = function () return player.class == "fighter" end,
          text = "Grad the book and run",
        },
        { eval = function () return player.class == "wizard" end,
          text = "Cast identify",
          code = function () player.arcaneGlow = true end,
          next = Script { Text {
                { s="narrator", "You seen an arcane glow."}
            }
          }
        }
    },
}

Actor {
    entityID = "_TEST_BOOK_READER",
    name = "Velma",
    -- what is this magic? sub-tables in LuRP?
    -- they are allowed, but always immutable.
    -- you can't change the value of a sub-table.
    accessory = {
        face = "glasses",
    }
}

Script {
    entityID = "_TEST_READING",
    npc = "_TEST_BOOK_READER",
    code = function()
        script.booksRead = 0
    end,
    Text {
        -- Subtle point: we can read sub-tables (accessory.face) but they are immutable.
        { s="narrator", "{npc.name} is reading a book. She is wearing {npc.accessory.face}.",
                        "{player.name} smiles at her."}
    },
    Text {
        code = function () script.booksRead = script.booksRead + 1 end,
        { s="narrator", "Finished with the book, {npc.name} closes the novel." }
    },
    Text {
        eval = function() return script.booksRead == 1 end,
        { s="{npc.name}", "Read the first book! (Books read = {script.booksRead})"}
    },
    Text {
        eval = function() return script.booksRead > 1 end,
        { s="{npc.name}", "That's {script.booksRead} books read."}
    }
}

Script {
    entityID = "_TEST_TEXT_TEST",
    code = function() script.pathA = true end,
    Text {
        { test = "{script.pathA}",  "This is pathA"},
        { test = "{~script.pathA}", "This is NOT pathA"},
        { test = "{script.pathB}",  "This is pathB"},
        { test = "{~script.pathB}", "This is NOT pathB"},
    }
}

Script {
    entityID = "ALT_TEXT_1",
    Text {
        code = function() script.more = true end,
[[
{s="Talker"}
I'm going to tell a story.

{s="Listener"}
Yay!

{s="Another" test={script.more}}
I want to hear too!

{s="YetAnother" test={~script.more}}
I want to hear too!
]]
    }
}

local scripted = {}
scripted.openChest =
Script {
    -- input:
    --   color = "color"
    Text {
        { s="narrator", "You see a chest with a {script.color} gem in the center." },
    },
}

Script {
    entityID = "_OPEN_RED_CHEST",
    CallScript {
        scriptID = scripted.openChest,
        code = function() script.color = "red" end
    }
}

Item {
    entityID = "SKELETON_KEY",
    name = "Skeleton key"
}

Script {
    entityID = "KEY_MASTER",
    Choices {
        { 
            eval = function() return not player:hasItem("SKELETON_KEY") end,
            text = "Get the key",
            code = function() player:addItem("SKELETON_KEY") end,
        },
        {
            text = "Do nothing"
        }
    },
    Text {
        { eval = function() return player:hasItem("SKELETON_KEY") end,
          s="narrator", "You have the Skeleton Key."}
    }
}

Script {
    entityID = "CHOICE_MODE_1_REPEAT",
    Text { { s="narrator", "Text before"} },
    Choices {
        action = "repeat",
        { text = "Choice 0", next = Text { { s="cm1", "about choice 0"} } },
        { text = "Choice 1", next = Text { { s="cm1", "about choice 1"} } },
        { text = "Choice 2", next = Choices {
                { text = "C2a", next = Text { { s="sub", "about c2a" }}},
                { text = "C2b", next = Text { { s="sub", "about c2b" }}}
            }
        },
        { text = "Done", next = "done" }
    },
    Text { { s="narrator", "Text after"} },
}

Script {
    entityID = "CHOICE_MODE_1_REWIND",
    Text { { s="narrator", "Text before"} },
    Choices {
        action = "rewind",
        { text = "Choice 0", next = Text { { s="cr1", "about choice 0"} } },
        { text = "Choice 1", next = Text { { s="cr1", "about choice 1"} } },
        { text = "Choice 2", next = Choices {
                { text = "C2a", next = Text { { s="sub", "about c2a" }}},
                { text = "C2b", next = Text { { s="sub", "about c2b" }}}
            }
        },
        { text = "Done", next = "done" }
    },
    Text { { s="narrator", "Text after"} },
}

Script {
    entityID = "CHOICE_MODE_1_POP",
    Text { { s="narrator", "Text before"} },
    Choices {
        action = "pop",
        { text = "Choice 0", next = Text { { s="cr1", "about choice 0"} } },
        { text = "Choice 1", next = Text { { s="cr1", "about choice 1"} } },
        { text = "Choice 2", next = Choices {
                { text = "C2a", next = Text { { s="sub", "about c2a" }}},
                { text = "C2b", next = Text { { s="sub", "about c2b" }}}
            }
        },
        { text = "Done", next = "done" }
    },
    Text { { s="narrator", "Text after"} },
}

Script {
    entityID = "CHOICE_MODE_2_TEST",
    Choices {
        entityID = "CHOICE_MODE_2",
        hint = "conversation",
        action = "repeat",
        { text = "Choice 0", next = Text { { s="cm2", "about choice 0"} } },
        { text = "Choice 1", next = Text { { s="cm2", "about choice 1"} } },
        { text = "Done", next = "done", eval = function() return Entity("CHOICE_MODE_2"):allTextRead() end },
    }
}

Item {
    entityID = "SWORD_01",
    name = "Sword",
}

Actor {
    entityID = "ACTOR_01",
    name = "The Actor 01",
    STR = 16,
    DEX = 10,
    attributes = {
        bard = true,
        sings = true,
    },
    items = { "SWORD_01" }
}

Script {
    entityID = "TEST_LUA_CORE",
    Script {
        code = function()
            local e = Entity("ACTOR_01")
            assert(e.entityID == "ACTOR_01", "npc should be ACTOR_01")
            assert(e.STR == 16, "npc.STR should be 16")
            e.STR = 17
            assert(e.STR == 17, "npc.STR should be 17 now")
            assert(e.attributes.bard, "npc should be a bard")
        end
    },
    Text { "waiting..."},
    Script {
        code = function()
            local e = Entity("ACTOR_01")
            assert(e.STR == 18, "npc.STR should be 18 now") -- fixme: this doesn't make sense
        end
    }
}

Script {
    entityID = "EMPTY_SCRIPT"
}

Script {
    entityID = "TEST_BATTLE_1",
    Battle {
        entityID = "TEST_BATTLE_1_CURSED_CAVERN",
        name = "Cursed Cavern",
        regions = {
            { "Stalagmites", 0, "light"},
            { "Shallow Pool", 8, "none"},
            { "Crumbling Stones", 16, "medium"},
            { "Wide Steps", 24, "none"},
        },
        Combatant {
            name = "Skeleton Warrior",
            count = 2,
            fighting = 6,
            shooting = 0,
            arcane = 0,
            bias = -1,
            items = { "LONGSWORD", "CHAINMAIL"},
        },
        Combatant {
            name = "Skeleton Archer",
            fighting = 4,
            shooting = 6,
            arcane = 0,
            items = { "SHORTSWORD", "BOW"},
        },
        Combatant {
            name = "Skeleton Mage",
            fighting = 0,
            shooting = 0,
            arcane = 6,
            bias = 1,
            powers = { "FIRE_BOLT"},
        },
    }
}

Combatant {
    entityID = "GOBLIN_1",
    name = "Goblin Warrior"
}

Combatant {
    entityID = "GOBLIN_2",
    name = "Goblin Archer"
}

Script {
    entityID = "TEST_BATTLE_2",
    Battle {
        entityID = "TEST_BATTLE_2_BATTLE",
        name = "Test Battle 2",
        regions = {
            { "Stalagmites", 0, "light"},
            { "Shallow Pool", 8, "none"},
            { "Crumbling Stones", 16, "medium"},
            { "Wide Steps", 24, "none"},
        },
        combatants = {
            { "GOBLIN_1", 2 },
            { "GOBLIN_2", 1 },
        }
    }
}