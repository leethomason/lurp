Actor {
    entityID = "testplayer",
    name = "Test Player",
    level = 1,
    maxHP = 10,
    hp = 9
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
            return player.arcane
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
            player.arcane = true
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
                        player.arcane = true
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
            return player.arcane
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
          code = function () player.arcane = true end,
          next = Script { Text {
                { s="narrator", "You seen an arcane glow."}
            }
          }
        }
    },
}

Script {
    entityID = "_TEST_BATTLE",
    Battle {
        player = "testplayer",
        enemy = Actor {
            name = "Skeleton",
            desc = "Bone warrior",
            class = "fighter",
            level = 1,
            boost = 0.7,
        }
    }
}

Actor {
    entityID = "_TEST_BOOK_READER",
    name = "Velma",
    accessory = {
        face = "glasses",
    }
}

Script {
    entityID = "_TEST_READING",
    code = function() script.booksRead = 0 end,
    Text {
        { s="narrator", "{npc.name} is reading a book. She is wearing {npc.accessory.face}.",
                        "{player.name} smiles at her."}
    },
    Text {
        code = function () script.booksRead = script.booksRead + 1 end,
        { s="narrator", "Finished with the book, {npc.name} closes the novel." }
    },
    Text {
        eval = function() return script.booksRead == 1 end,
        { s="npc", "Read the first book! (Books read = {script.booksRead})"}
    },
    Text {
        eval = function() return script.booksRead > 1 end,
        { s="npc", "That's {script.booksRead} books read."}
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
            e = Entity("ACTOR_01")
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
            assert(e.STR == 18, "npc.STR should be 18 now")
        end
    }
}

Script {
    entityID = "TEST_BATTLE_1",
    Battle {
        player = "testplayer",
        enemy = Actor {
            entityID = "ENEMY_01",
            name = "Skeleton",
            desc = "Skeleton warrior",
            strengh = "d6",
        }
    }
}
