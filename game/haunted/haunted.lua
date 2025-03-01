local lume = require "lume"
local lurp = require "lurp"

local MAX_PLAYERS = 4
local NUM_ROOMS = 6
local NUM_GREEN = 4
local NUM_RED = 2

-- fixme: Event Cards: where do the cards come from?
-- fixme: Event Cards: where are they discarded?
-- fixme: Event Cards: how are they drawn?
-- fixme: Item Chits: where do the cards come from?
-- fixme: Item Chits: where are they discarded?
-- fixme: Item Chits: how are they drawn?
-- fixme: put the fear values in the event card
-- fixme: need a hand
-- fixme: items in hand destroy ghosts/poltergeists
-- fixme: win condition

local board = {
    { name = "Dining", x = 0, y = 5, w = 13, h = 3, connect = { "Main Hall" } },
    { name = "Main Hall", x = 19, y = 5, w = 12, h = 3, connect = { "Ballroom", "Garden", "Lounge", "Foyer", "Dining" } },
    { name = "Lounge", x = 36, y = 5, w = 10, h = 3, connect = {"Garden", "Main Hall"} },
    { name = "Foyer", x = 19, y = 10, w = 12, h = 2, connect = {"Main Hall"}  },

    { name = "Ballroom", x = 19, y = 0, w = 12, h = 3, connect = { "Garden", "Main Hall" } },
    { name = "Garden", x = 36, y = 0, w = 10, h = 3, connect = { "Ballroom", "Main Hall", "Lounge" } },
}

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

    -- create the chits
    table.insert(box.greenItemChits, Card:new("Green Item", "Holy Water", {}))  -- destroy a ghost or poltergeist
    table.insert(box.greenItemChits, Card:new("Green Item", "Ward", {}))  -- destroy a ghost
    table.insert(box.greenItemChits, Card:new("Green Item", "Light", {}))  -- fear -1
    table.insert(box.greenItemChits, Card:new("Green Item", "Salt", {}))  -- destroy a poltergeist

    table.insert(box.redItemChits, Card:new("Red Item", "Magic Grimoire", {}))  -- win the game
    table.insert(box.redItemChits, Card:new("Red Item", "Cursed Doll", {}))  -- fear +1
end

function onSetupGame(game, box, players)
    lume.shuffle(box.eventCards)
    lume.shuffle(box.greenItemChits)
    lume.shuffle(box.redItemChits)

    for i=1, NUM_ROOMS do
        box.eventCards[i].pos = board[i].name
    end    
    for i=1, NUM_GREEN do
        box.greenItemChits[i].pos = board[i].name
    end
    for i=1, NUM_RED do
        box.redItemChits[i].pos = board[i + NUM_GREEN].name
    end
end

function onInit(game, box, players)
    --math.randomseed(os.time())
    math.randomseed(1357)
    registerCard("event", handleEventCard)
end

function handleEventCard(game, box, players, player, card)
    if card.name == "Poltergeist" then
        player.fear = player.fear + 1
    elseif card.name == "Ghost" then
        player.fear = player.fear + 2
    end
end
