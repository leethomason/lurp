local lume = require "lume"

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

function printMeeples(meeples)
    print("printMeeples")
    for _, v in ipairs(meeples) do
        print("meeples", v.name, v.color)
    end
end

function printBox(box)
    print("printBox")
    printMeeples(box.meeples)
end

-- Game creation: yes, 1st
-- Game load: no
-- @return maximum number of players
function onSetupBox(box)
    local colors = { "red", "green", "blue", "yellow" }
    for i=1, MAX_PLAYERS do
        table.insert(box.meeples, Meeple:new("player", "P" .. i, colors[i]))
    end

    table.insert(box.meeples, Meeple:new("place", "green"))

    return MAX_PLAYERS
end

-- Game creation: yes, 2nd
-- Game load: no
function onSetupGame(players, box)
    local meeples = lume.filter(box.meeples, function(meeple) return meeple.name == "player" end)
    assert(#meeples == MAX_PLAYERS)  -- not generally true, but is for this game

    for i=1, #players do
        meeples[i].pos = "Foyer"
        lume.push(players[i].meeples, meeples[i])
        players[i].fear = Counter:new(0, 0, 4)
    end
end

-- Game creation: yes, 3rd
-- Game load: yes, 1st
function onInit(players, box)
    for i=1, #players do
        players[i].fear.onMax = function() 
            print("'fear' max: Game Over for player " .. i)
            --GameOver()
            PlayerOver(players[i].index)
        end
    end
end

-- Calls when a player starts their turn.
function onStartTurn(player)
    print("Lua onStartTurn", player.index)
end

-- Called when a turn is ending. 
function onEndTurn(player)
    print("Lua onEndTurn", player.index)
    return nil
end

function isMoveAllowed(player, meeple, start, dst)
    print("Lua isMoveAllowed", player.index, meeple.name, start.name, dst.name)
    return true
end

function onMeepleMoved(player, meeple, start, dst)
    print("Lua onMoveMeeple", player.index, meeple.name, start.name, dst.name)
    player.fear:inc()
end

