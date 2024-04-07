--[[
    A very simple Script that introduces a scenario.
]]

-- There always has to be a "player" Actor for the scripts to work.
Actor {
    entityID = "player",
    name = "Blackwood"
}

-- The Zone is juts a simple container for the Script.
-- The Interaction is `required = true` to automatically start the Script.
Zone {
    entityID = "ZONE",
    name = "Zephyr Hotel",

    Room {
        name = "Casino",
        Interaction {
            entityID = "CASINO_INTERACTION",
            required = true,
            next = "CASINO_SCRIPT"
        }
    }
}

-- The heart of what this does.
Script {
    entityID = "CASINO_SCRIPT",
    name = "Casino Script",

    -- The console driver will display text with a speaker vs. without differently. The
    -- narrator (in this simple example) is a blank, but not empty, name.
    Text {
        s = " ", "As the elevator descends from the top floors of the Zephyr Hotel, you adjust your..."
    },
    -- write flags to the player so we know what they are wearing
    Choices {
        {
            text = "Tuxedo",
            code = function () player.tux = true player.dress = false end
        },
        {
            text = "Dress",
            code = function () player.dress = true player.tux = false end
        }
    },
    -- examples of testing what text to display
    Text {
        { s = " ", test = "{player.tux}", "...in the mirror and double-check the semi-auto inside the jacket."},
        { s = " ", test = "{player.dress}", "...in the mirror and double-check the semi-auto inside your purse." },
        { s = " ", "The doors open and you walk out onto the Casino floor. Walking across the noise "..
          "and lights of the Casino you approach the roulette table. The same table - not "..
          "coinicidentally - where Victor Xero is playing."},
        { s = "{player.name}", "Waitress, I'll have a..."}
    },
    -- this writes to the 'script' object, which only lives as long as the Script does
    Choices {
        {
            text = "Martini",
            code = function () script.drink = "martini" end
        },
        {
            text = "Whiskey",
            code = function () script.drink = "whiskey" end
        }
    },
    -- text substitution
    Text {
        { s = " ", "The wheel spins as you await your drink. You watch Victor as the ball bounces around the wheel. "..
           "What is Victor's real game tonight?" },
        { s = " ", "The waitres places your {script.drink} on the table."},
    }
}