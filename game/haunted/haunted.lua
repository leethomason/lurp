local lume = require "lume"
local lurp = require "lurp"

local MAX_PLAYERS = 4

local board = {
    { name = "Ballroom", x = 19, y = 0, w = 12, h = 3, connect = { "Garden", "Main Hall" } },
    { name = "Garden", x = 36, y = 0, w = 10, h = 3, connect = { "Ballroom", "Main Hall", "Lounge" } },
    { name = "Dining", x = 0, y = 5, w = 13, h = 3, connect = { "Main Hall" } },
    { name = "Main Hall", x = 19, y = 5, w = 12, h = 3, connect = { "Ballroom", "Garden", "Lounge", "Foyer", "Dining" } },
    { name = "Lounge", x = 36, y = 5, w = 10, h = 3, connect = {"Garden", "Main Hall"} },
    { name = "Foyer", x = 19, y = 10, w = 12, h = 2, connect = {"Main Hall"}  },
}

-- Can be called any time.
-- Note: conistency; the board can expand, but not change. (true? fixme)
function onFetchBoard()
    return board
end

function onMaxPlayers()
    return MAX_PLAYERS
end

function onSetupBox(game, box)
    -- create the player meeples
    local colors = { "red", "green", "blue", "yellow" }
    for i=1, MAX_PLAYERS do
        table.insert(box.meeples, Meeple:new("player", "P" .. i, colors[i]))
    end

    -- add the decks
    box.eventCards = {}
    box.greenItemChits = {}
    box.redItemChits = {}

    -- create the event cards    
    for i=1, 4 do
        table.insert(box.eventCards, Card:new("event", "Poltergeist", "white")) -- fear +1
    end
    for i=1, 4 do
        table.insert(box.eventCards, Card:new("event", "Ghost", "white")) -- fear +2
    end
    for i=1, 4 do
        table.insert(box.eventCards, Card:new("event", "Just a noise...", "white"))
    end
end

function onSetupGame(game, box, players)
    lurp.shuffle(box.eventCards)
    lurp.shuffle(box.greenItemChits)
    lurp.shuffle(box.redItemChits)
end

function onInit(game, box, players)
    registerCard("event", handleEventCard)
end

function handleEventCard(game, box, players, player, card)
    if card.name == "Poltergeist" then
        player.fear = player.fear + 1
    elseif card.name == "Ghost" then
        player.fear = player.fear + 2
    end
end

