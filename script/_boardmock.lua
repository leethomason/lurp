local lume = require "lume"

local MAX_PLAYERS = 4

local board = {
    { name = "A", x = 0, y = 0, w = 8, h = 4, connect = { "B" }},
    { name = "B", x = 10, y = 0, w = 8, h = 4, connect = { "A" }}
}

-- called as needed
function onFetchBoard()
    return board
end

-- called as needed
function onMaxPlayers()
    return MAX_PLAYERS
end

-- called once at start of game
function onSetupBox(game, box)
    box.init = true
    for i=1, MAX_PLAYERS do
        table.insert(box.meeples, Meeple:new("player", "Pn", "gray"))
    end
end

-- called once at start of game, after onSetupBox
-- fill in players
function onSetupGame(game, box, players)
    game.setup = true

    for i=1, #players do
        lume.push(players[i].meeples, box.meeples[i])
    end
end

-- called after onSetupBox and onSetupGame, or after load
function onInit(game, box, players)
    game.init = true
end

function onStartTurn(game, player)
    player.start = player.start or 0
    player.start = player.start + 1
end

function onEndTurn(game, player)
    player.start = player.start - 1
end

function isMoveAllowed(game, player, meeple, start, dst)
    return true
end

function onMeepleMoved(game, player, meeple, start, dst)
end
