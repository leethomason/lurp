-- Required
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
    name = "Emmerson",
    level = 1,
    items = { { "GOLD", 20 } },
}

Actor {
    entityID = "Giselle",
    name = "Giselle",
}

local startScript = Text {
    name = "STARTING_TEXT",
[[
You dream of stormy waves on a vast gray sea. As you fly over the whitecaps whipped by the 
wind, you feel the cold spray one your face. In the distance, the surface is boiled by something 
vast rising from the deep.

Your feet touch the stone tile of a small ancient temple rising just above the sea. The waves break
and splash across the temple floor. Inlaid in the floor is a symbol you recognize. The Octal Eye. 

You awake.

You splash water on your face. You remember the symbol of the Octal Eye from the tomb in Cairo.

You need some coffee and fresh air.
]],
}

local morningCoffee = Text {
    name = "MORNING_COFFEE_SF",
[[
You enter your local coffee shop. The barista sees you come in. "Morning {player.name}. Good to see
you. A woman was looking for you a few minutes ago. Left you a note." She hands you a folded piece
of paper.

You order your usual coffee and sit at a table outside the cafe. The morning sun is warms you 
even though the San Francisco air is cool. The note reads:

    Cairo. Meet me at the library. Research section in back. -G

Giselle was with you on the mission to Cairo. A good researcher, a good shot, and a good friend.
You both hoped to never see the Octal Eye again after that cursed tomb and its visions. But here 
you are. She must have had the same dream - vision - last night.

You finish the coffee. You suspect you'll need it.
]],
    code = function() zone.hadMorningCoffee = true end
}

local meetGiselleAtLibrary = Text {
    name = "MEET_GISELLE_AT_LIBRARY",
[[
You make your way through the doors of the old, regal Library. You find Giselle in 
the back research rooms.

"{player.name} it's goood to see you!" she says with a warm hug. She's easy on the eyes in a 
wearing a dress suitable for academia and at home in the library. "You had the vision as well?"

"I did. Wish I hadn't. But I did. Can't shake it. It means...?"

"An Old One is rising. I'm sure of it." Worry crosses her face. "The tomb in Cairo wasn't just a
cursed place - it's the power of the Old One leaking into our world." She pauses. "We have to stop
it {player.name}. With me on this? Off on another adventure?"

You nod. "I'm in. What's our next step?"

"Well there we have some good luck. Shahir - the researcher or mystic or whatever he was - left
his work when he passed on. His ranting about 'saving us from the Old Ones' seems plausible
now. This very University and this very Library has the journal. We just need to steal it."

"Steal it?"

"Steal it. No time for red tape. I've got a plan. I'll distract the librarian. You get the journal.
Assuming we save the world we'll give it back to them. Meet me at the cafe for lunch."

You smile. "I've always liked your style. Can I get a hair pin? I wasn't prepared for a heist."

Giselle hands you a pin, stands up, and calls out, "Librarian Deweyish? Mr. Deweyish? Or is that
Dr. Deweyish? I have some questions about finding books in sub-section 3."
]],
    code = function() player:addItem("HAIRPIN", 1) end
}

local coffeeWithGiselle = Script {
    name = "COFFEE_WITH_GISELLE",
    Text { 
        code = function() player:removeItem("SHAHIR_JOURNAL") end,
[[
Giselle is sipping tea at the cafe. She smiles when you arrive and put the journal on the table.
"Nice work {player.name}. Although I'll give "myself some credit for talking to that bore Deweyish.
Grab youself a sip and a pastry. I want a look at this thing."

Giselle starts reading through the journal while you order tea and samosas. You make notes in your 
own journal, collecting your thoughts on the turn of events.

"A compass and a key." She says. You look up from your musings. She has notes scribbled on papers 
and bookmarks in the journal.
]],
    },
    Choices {
        name = "COFFEE_CHOICES_IN_SF",
        action = "repeat",
        {
            text = "A compass?",
            next = Text { [[
"A compass that always points to the Unknown Temple. The only way to find it. Last seen
in the hands of the Mad Pirate."

"Colorful name" you say. "Where do we find this character?"

"Dead, thankfully. His bones rest in the Blue Cave in Curacao."

You smile. "Worse places to chase ancient artifacts."
]]
            },
        },
        {
            text = "The key?",
            next = Text { [[
"The key to the Unknown Temple. Shahir didn't know if it summoned or blocked the Old One. Or
what it did at all, really. But given what we've seen, let's hope it stops the Old One at the 
temple." Giselle pauses. "A mausoleum under Paris. Shahir even gave us a helpful map."
]]
            }
        },
        {
            text = "The temple?",
            next = Text { [[
"I'm not sure yet." You see a look of concern flash across Giselle's face. "We seem to have
a lead on finding it and the key is obviously critical. But what do we do when we get there?
I need some more time with the journal. And I need to talk to Dr. Zeyad."

"Back to Cairo?"

"Back to Cairo. I think we have to split up for a bit. You go to Curacao and Paris, I'll go
to Cairo. I'll send a telegraph when I have information."
]]
            }
        },
        {
            eval = function() return Entity("COFFEE_CHOICES_IN_SF"):allTextRead() end,
            text = "Adventure awaits"
        },
    },
    Text {[[
You stand up. "I'll get by bags and get going. I'll see you in Cairo."

"Thanks {player.name}. I think you'll need to pack your trusty six gun for this.
Be careful and I'll see you soon.
]]
    },
    code = function() zone.needRevolver = true end
}

Zone {
    name = "San Francisco",

    Room {
        name = "Your Apartment",
        desc = "A small one bedroom in the Mission district. A bit run down but with a beautiful view.",
        Interaction { required = true, next = startScript },
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
            required = true,
            next = meetGiselleAtLibrary
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
            next = morningCoffee
        },
        Interaction {
            required = true,
            eval = function() return player:hasItem("SHAHIR_JOURNAL") end,
            name = "Giselle is sipping coffee.",
            actor = "Giselle",
            next = coffeeWithGiselle
        }
    },
    Room {
        entityID = "SFO",
        name = "SF Intenational Aeroport",
        desc = "San Francisco's airport with connections all over the world.",
        Interaction {
            required = true,
            eval = function() return player:hasItem("REVOLVER") end,
            next = Script {
                Text { "You board the plane...or would, if this demo was complete. Thanks for playing!" ,
                       code = function() EndGame("Game Over") end, "",
                },
            }
        }
    },

    EdgeGroup {
        name = "{dest}",
        rooms = { "Your Apartment", "SF_LIBRARY", "SF_CAFE", "SFO" },
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
