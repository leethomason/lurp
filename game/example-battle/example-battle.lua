Power {
    entityID = "FIRE_BOLT",
    name = "Fire Bolt",
    effect = "bolt",
    cost = 1,
    range = 2,      -- multiplier: 0 touch, 1 short, 2 medium, 3 long
    strength = 1,   -- multiplier: 1 standard, 2 strong, 3 very strong
}

Power {
    entityID = "ARMOR_BOOST",
    name = "Armor++",
    effect = "armor",
    cost = 1,
    range = 0,      -- multiplier: 0 touch, 1 short, 2 medium, 3 long
    strength = 1,   -- multiplier: 1 standard, 2 strong, 3 very strong
}

Power {
    entityID = "HEAL",
    name = "Heal",
    effect = "heal",
    cost = 1,
    range = 0,
    strength = 1,
}

-- Item that is a melee weapon
Item {
    entityID = "LONGSWORD",
    name = "longsword",
    range = 0,          -- melee
    damage = "d8",      -- damage die
}

-- Item that is a melee weapon
Item {
    entityID = "SHORTSWORD",
    name = "shortsword",
    range = 0,
    damage = "d6",
}

-- Item that is a ranged weapon
Item {
    entityID = "BOW",
    name = "bow",
    range = 24,             -- medium range
    damage = "d6",          -- damage die
}

-- Item that is armor
Item {
    entityID = "CHAINMAIL",
    name = "chainmail",
    armor = 3,
}

-- Item that is armor
Item {
    entityID = "LEATHER",
    name = "leather",
    armor = 2,
}

-- Item that is armor
Item {
    entityID = "ROBE",
    name = "robe",
    armor = 1,
}

-- Define the player, noting that the script will choose the bonuses later
Actor {
    entityID = "player",
    name = "Grom",

    wild = true,    -- flags as a more powerful (and important) actor for combat
    fighting = 4,   -- from 2-20, usually even, 2 is unskilled, 4 is basic, 12 is incredible
    shooting = 4,
    arcane = 4,

    -- There is a limitation in the current version of LuRP that tables on Entinies
    -- are only read only. We can do this, BUT the powers can never change and must
    -- be set here in the declaration.
    powers = { "FIRE_BOLT", "ARMOR_BOOST", "HEAL" },
}

Zone {
    entityID = "BATTLE_ZONE",
    name = "Battle Zone",

    Room {
        name = "Lobby",
        desc = "Outside the Battle Arena",

        -- We want the player to start in the lobby, but choose
        -- fighter / shooter / arcane and then go straight to
        -- the battle. Required Intereraction + Choices do that.
        Interaction {
            required = true,
            next = Script {
                Text {
                    "Welcome warrior to the Arena. Choose your gear."
                },
                Choices {
                    { text = "Knight Gear (fighting)", next = "KNIGHT_SCRIPT" },
                    { text = "Archer (shooting) ", next = "ARCHER_SCRIPT" },
                    { text = "Wizard (arcane)", next = "WIZARD_SCRIPT" }
                },
                Text {
                    "To the Arena!",
                    code = function() MovePlayer("BATTLE_ZONE_ARENA", true) end
                }
            }
        }
    },
    Room {
        entityID = "BATTLE_ZONE_ARENA",
        name = "Arena",
        Interaction {
            required = true,
            next = "SKELETON_BATTLE"
        }
    }
}

Script {
    entityID = "KNIGHT_SCRIPT",
    code = function()
        player.fighting = 6
        player.shooting = 6
        player:addItem("LONGSWORD")
        player:addItem("CHAINMAIL")
        player:addItem("BOW")
    end
}

Script {
    entityID = "ARCHER_SCRIPT",
    code = function()
        player.fighting = 4
        player.shooting = 8
        player:addItem("SHORTSWORD")
        player:addItem("LEATHER")
        player:addItem("BOW")
    end
}

Script {
    entityID = "WIZARD_SCRIPT",
    code = function()
        player.arcane = 10
        player:addItem("SHORTSWORD")
        player:addItem("LEATHER")
    end
}

Script {
    entityID = "SKELETON_BATTLE",
    Battle {
        name = "Skeleton Battle",
        regions = {
            { "Stone Barriers", 0, "medium"},
            { "Open Space", 10, "none"},
            { "Old Columns", 20, "medium"},
            { "Crumbling Arches", 30, "light"},
        },
        Combatant {
            name = "Skeleton Warrior",
            count = 2,
            fighting = 6,
            shooting = 0,
            arcane = 0,
            bias = -1,
            items = { "LONGSWORD", "LEATHER"},
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
    },
    Text {
        "You are victorious!"
    }
}
