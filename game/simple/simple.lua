Item {
    entityID = "GOLD",
    name = "Gold"
}

Actor {
    entityID = "player",
    name = "Finnian"
}

Zone {
    entityID = "TAVERN",
    name = "An Old Tavern",
    Room {
        name = "The Bar",
        Interaction {
            name = "The Worn Map",
            required = true,
            next = Text {
                "The worn map from the odd peddler has led you here. If it's correct - something that is far "..
                "from certain - then the entrance to the old ruins is...right behind the bar? Huh. "..
                "The place is empty enough. Just need to distract the bartender. You are a bard, after all."
            }
        }
    }
}

local z = "SimpleZone"
Zone {
    entityID = z,
    name = "Simple Zone",

    Room {
        entityID = z.."Brick Room",
        name = "Brick Room",
        Container {
            name = "Wood Chest",
            items = {
                { "GOLD", 10 },
                "SKELETON_KEY"
            }
        },
    },
    Room {
        entityID = z.."Stone Room",
        name = "Stone Room",
        Interaction {
            name = "Wall Lever",
            next = Text {
                "You pull the lever and hear a click.", code = function()
                    local door = Entity(z.."IronDoor")
                    door.locked = not door.locked
                end
            }
        }
    },
    Room {
        entityID = z.."SecretChamber",
        name = "Secret Chamber",
    },
    Edge {
        name = "Hallway",
        room1 = z.."Brick Room",
        room2 = z.."Stone Room",
        dir = "n",
        locked = true,
        key = "SKELETON_KEY",
    },
    Edge {
        entityID = z.."IronDoor",
        room1 = z.."Stone Room",
        room2 = z.."SecretChamber",
        name = "Iron Door",
        locked = true,
        dir = "e",
    }
}