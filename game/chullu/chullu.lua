Config = {
    gameVersion = "0.1",
    gameAuthor = "Game Author",
    gameDescription = "A text adventure game",

    size = { 1920, 1080},
    clearColor = { 0, 0, 0 },
    frames = {
        { 0, 0, 1920, 1080, "background" },
        { 40, 40, 500, 1000, "image"},          -- fixme
        { 540, 40, 1340, 1000, "text", type="text", opaque=true},  -- fixme
        { 40, 1040, 1840, 40, "status"},         -- fixme
    }
}

LoadMD("chullu.md")

Item {
    entityID = "GOLD",
    name = "Greenbacks"
}

Item {
    entityID = "SHAHIR_JOURNAL",
    name = "Shahir's Journal",
}

Item {
    entityID = "REVOLVER",
    name = "Revolver",
}

Item {
    entityID = "HAIRPIN",
    name = "Hairpin",
}

Actor {
    entityID = "player",
    name = "Emerson",
    level = 1,
    items = { { "GOLD", 20 } },
}

Actor {
    entityID = "Giselle",
    name = "Giselle",
}

Script {
    entityID = "COFFEE_WITH_GISELLE",
    CallScript {
        code = function() player:removeItem("SHAHIR_JOURNAL") end,
        scriptID = "COFFEE_WITH_GISELLE_INTRO",
    },
    Choices {
        entityID = "COFFEE_CHOICES_IN_SF",
        action = "repeat",
        {
            text = "A compass?",
            next = "COFFEE_WITH_GISELLE_COMPASS"
        },
        {
            text = "The key?",
            next = "COFFEE_WITH_GISELLE_KEY"
        },
        {
            text = "The temple?",
            next = "COFFEE_WITH_GISELLE_TEMPLE"
        },
        {
            -- note this trick only works because this option doesn't have a Text entity.
            -- FIXME: need a better way to handle conversation flows
            eval = function() return Entity("COFFEE_CHOICES_IN_SF"):allTextRead() end,
            text = "Adventure awaits"
        },
    },
    CallScript {
        scriptID = "COFFEE_WITH_GISELLE_ADVENTURE",
        code = function() zone.needRevolver = true end,
    }
}

Zone {
    entityID = "SAN_FRANCISCO",
    name = "San Francisco",

    Room {
        entityID = "APARTMENT",
        name = "Your Apartment",
        desc = "A small one bedroom in the Mission district. A bit run down but with a beautiful view.",
        Interaction { required = true, next = "STARTING_TEXT" },
        Container {
            eval = function() return zone.needRevolver end,
            name = "Closet",
            items = { { "REVOLVER", 1 } },
        }
    },
    Room {
        entityID = "SF_LIBRARY",
        name = "The SF City Library",
        desc = "An old library of marble and wood.",
        Interaction {
            eval = function () return zone.hadMorningCoffee end,
            code = function() player:addItem("HAIRPIN", 1) end,
            required = true,
            next = "MEET_GISELLE_AT_LIBRARY"
        },
        Interaction {
            entityID = "SF_LIBRARY_RESTRICTED_INTERACTION",
            name = "The Restricted Section",
            next = Script {
                Choices {
                    {   eval = function() return player:hasItem("HAIRPIN") end,
                        text = "Pick the lock",
                        next = Script {
                            Text {
                                { "You pick the lock." },
                                code = function()
                                    Entity("LIBRARY_RESTRICTED_DOOR").locked = false
                                end,
                            },
                        }
                    },
                    { text = "Shoot the lock off",
                        next = Script {
                            Text {
                                { "This is a library! You can't do that." },
                                { eval = function() return not player:hasItem("REVOLVER") end,
                                "Not to mention you don't have a revolver." },
                            }
                        }
                    },
                    { text = "Step away" }
                }
            }
        },
        Interaction {
            required = true,
            eval = function ()
                return player:hasItem("SHAHIR_JOURNAL")
            end,
            next = Text { "The door locks behind you." },
            code = function()
                Entity("LIBRARY_RESTRICTED_DOOR").locked = true
                player:removeItem("HAIRPIN")
            end,
        }
    },
    Room {
        entityID = "SF_LIBRARY_RESTRICTED",
        name = "The Restricted Section",
        desc = "A small cramped room with shelves of all kinds of books.",
        Container {
            name = "New arrivals shelf",
            items = { "SHAHIR_JOURNAL" },
        }
    },
    Room {
        entityID = "SF_CAFE",
        name = "The Black Soul Cafe",
        desc = "A cafe with expensive dark coffee.",
        Interaction {
            required = true,
            eval = function() return not zone.hadMorningCoffee end,
            code = function() zone.hadMorningCoffee = true end,
            next = "MORNING_COFFEE_SF"
        },
        Interaction {
            required = true,
            eval = function() return player:hasItem("SHAHIR_JOURNAL") end,
            name = "Giselle is sipping coffee.",
            npc = "Giselle",
            next = "COFFEE_WITH_GISELLE"
        }
    },
    Room {
        entityID = "SFO",
        name = "SF International Aeroport",
        desc = "San Francisco's airport with connections all over the world.",
        Interaction {
            required = true,
            eval = function() return player:hasItem("REVOLVER") end,
            next = Script {
                Text { "You board the plane...or would, if this demo was complete. Thanks for playing!" },
                Script { code = function() EndGame("Game Over", 0) end }
            }
        }
    },

    EdgeGroup {
        rooms = { "APARTMENT", "SF_LIBRARY", "SF_CAFE", "SFO" },
    },

    Edge {
        entityID = "LIBRARY_RESTRICTED_DOOR",
        name = "Library gate",
        dir = 'n',
        room1 = "SF_LIBRARY",
        room2 = "SF_LIBRARY_RESTRICTED",
        locked = true,
    }
}
