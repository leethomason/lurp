--[[
Battles take a lot of declaration. Even this simple example has a lot of
setup. 

The actual script lets you choose a "class": Knight, Archer, or Wizard. 
And then you are teleported to the Arena to fight enemies of choice.

LuRP uses a subset of the Savage Worlds (SW) rules. (Specifically the SWADE
rules.) One HUGE advantage is that you can buy the rule book and simply
look up gear, costs, that sort of thing. SW is more well rounded and a larger
ruleset than LuRP, but I've tried to make it as re-usable as possible.

NOTE: In SW, the distance in the rules is specified in tabletop inches.
      1" = 2 yards. LuRP uses yards / tabletop inches interchangably,
      so there is a factor of 2 error, than in practice seems to wash 
      out. So when moving from SW to LuRP, the distance value is used as-is.
]]--

--[[
    Powers define the arcane abilities available in this example game
]]

Power {
    entityID = "FIRE_BOLT",
    name = "Fire Bolt",
    effect = "bolt",    -- there are a number of effects, which you can look up in the script.md docs
    cost = 1,       -- cost to use, straight from SW
    range = 2,      -- multiplier: 0 touch, 1 short, 2 medium, 3 long. The "smarts" multiplier from SW.
    strength = 1,   -- multiplier: 1 standard, 2 strong, 3 very strong
}

Power {
    entityID = "ARMOR_BOOST",
    name = "Armor++",
    effect = "armor",
    cost = 1,
    range = 0,      -- touch (same region)
    strength = 1,
}

Power {
    entityID = "HEAL",
    name = "Heal",
    effect = "heal",
    cost = 1,
    range = 0,
    strength = 1,
}

--[[
     Items of all types: armor, weapons, keys, money, etc. that can be used in this example
]]--
-- Item that is a melee weapon
Item {
    entityID = "LONGSWORD",
    name = "longsword",
    range = 0,          -- melee so range is 0
    damage = "d8",      -- damage die from SW
}

-- Item that is a melee weapon
Item {
    entityID = "SHORTSWORD",
    name = "shortsword",
    range = 0,          -- melee so range is 0
    damage = "d6",      -- damage die from SW
}

-- Item that is a ranged weapon
Item {
    entityID = "BOW",
    name = "bow",
    range = 24,             -- medium range. In SW, this is the medium range in (table top) inches, 
                            -- which is the value listed in the gear tables.
    damage = "d6",          -- damage die from SW
}

-- Item this is a ranged weapon
Item {
    entityID = "PULSE_RIFLE",
    name = "pulse rifle",
    range = 60,
    damage = "3d6"
}

-- Item that is a mellee weapon
Item {
    entityID = "PLASMA_SWORD",
    name = "plasma sword",
    range = 0,
    damage = "d6"
}

-- Item that is armor
Item {
    entityID = "RECON_ARMOR",
    name = "recon armor",
    armor = 4,
}

-- Item that is armor
Item {
    entityID = "CHAINMAIL",
    name = "chainmail",
    armor = 3,              -- toughness provided from armor (again, straight from SW)
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

--[[
    Define the Actors in this simple game. 
    NOTE: there is only the player, because enemies / monsters / battles are Combatants,
          which are transient. Actors persist for the game. Combatants exist for the battle.
]]
-- Define the player, noting that the script will choose the bonuses later
Actor {
    entityID = "player",
    name = "Grom",

    wild = true,    -- flags as a more powerful (and important) actor for combat (see SW rules)
    fighting = 4,   -- from 2-20, usually even, 2 is unskilled, 4 is basic, 12 is incredible
    shooting = 4,
    arcane = 4,

    -- There is a limitation in the current version of LuRP that tables on Entinies
    -- are only read only. We can do this, BUT the powers can never change and must
    -- be set here in the declaration.
    powers = { "FIRE_BOLT", "ARMOR_BOOST", "HEAL" },
}

--[[
    Define the Zones and Rooms
    NOTE: In this example, the rooms aren't connected. After a choice is made (I want to be a 
    Knight) the player is teleported to the Arena to fight.
]]
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
                    { text = "Wizard (arcane)", next = "WIZARD_SCRIPT" },
                    { text = "Space Marine (fighting, shooting)", next = "SPACE_MARINE_SCRIPT" },
                    { text = "Star Mage (arcane, fighting)", next = "STAR_MAGE_SCRIPT"}
                },
                Text {
                    "To the Arena!"
                },
                Choices {
                    { text = "Small Skeleton Battle", next = "SMALL_SKELETON_BATTLE"},
                    { text = "Skeleton Battle", next = "SKELETON_BATTLE"}
                },
                Text {
                    -- You can only see this text if you win. (Otherwise the game was over.)
                    { "Congratulations gladiator! You have won the battle!",
                       code = function() EndGame("Game Over", 1) end }
                }

            }
        }
    }
}

--[[
    The Scripts that make the events happen.
]]

Script {
    entityID = "KNIGHT_SCRIPT",
    code = function()
        player.fighting = 8
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
    entityID = "SPACE_MARINE_SCRIPT",
    code = function()
        player.fighting = 6
        player.shooting = 8
        player:addItem("PLASMA_SWORD")
        player:addItem("RECON_ARMOR");
        player:addItem("PULSE_RIFLE")
    end
}

Script {
    entityID = "STAR_MAGE_SCRIPT",
    code = function()
        player.arcane = 10
        player.fighting = 8
        player:addItem("PLASMA_SWORD")
    end
}

-- This Script wraps up a Battle and some Text about victory. (If
-- you are defeated, the game is over, so there's only one in-game outcome.)
Script {
    entityID = "SKELETON_BATTLE",
    Battle {
        name = "Skeleton Battle",

        -- The regions of the battlefield - a board in 1D
        regions = {
            { "Stone Barriers", 0, "medium"},   -- name, distance, cover. Player starts here.
            { "Open Space", 10, "none"},
            { "Old Columns", 20, "medium"},
            { "Crumbling Arches", 30, "light"}, -- enemies start here
        },
        Combatant {
            name = "Skeleton Warrior",
            count = 2,                      -- number of this type of combatant
            fighting = 6,
            shooting = 0,
            arcane = 0,
            bias = -1,                      -- bias makes a "little better" (+1) or "little worse" (-1)
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
}

Script {
    entityID = "SMALL_SKELETON_BATTLE",
    Battle {
        name = "Small Skeleton Battle",

        -- The regions of the battlefield - a board in 1D
        regions = {
            { "Stone Barriers", 0, "medium"},   -- name, distance, cover. Player starts here.
            { "Open Space", 10, "none"},
            { "Old Columns", 20, "medium"},
            { "Crumbling Arches", 30, "light"}, -- enemies start here
        },
        Combatant {
            name = "Skeleton Warrior",
            count = 1,                      -- number of this type of combatant
            fighting = 6,
            shooting = 0,
            arcane = 0,
            bias = 0,                      -- bias makes a "little better" (+1) or "little worse" (-1)
            items = { "LONGSWORD", "LEATHER"},
        },
        Combatant {
            name = "Skeleton Archer",
            fighting = 4,
            shooting = 6,
            arcane = 0,
            bias = -1,
            items = { "SHORTSWORD", "BOW"},
        },
    },
}
