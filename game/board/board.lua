-- Rules / assumptions
-- 1. a. The board is a graph of rooms. OR
--    b. The board is a grid of rooms. 
--    Can't mix graphs and grids.
-- 2. The board is static. Rooms don't move. Although the can be blocked. See onMoveMeeple()
-- 3. A player can have n meeples. TBD: move each meeple? actions per meeple?


local MAX_PLAYERS = 4

local board = {
    { name = "Ballroom", x = 19, y = 0, w = 12, h = 3, connect = { "Garden", "Main Hall" } },
    { name = "Garden", x = 36, y = 0, w = 10, h = 3, connect = { "Ballroom", "Main Hall", "Lounge" } },
    { name = "Dining", x = 0, y = 5, w = 13, h = 3, connect = { "Main Hall" } },
    { name = "Main Hall", x = 19, y = 5, w = 12, h = 3, connect = { "Ballroom", "Garden", "Lounge", "Foyer", "Dining" } },
    { name = "Lounge", x = 36, y = 5, w = 10, h = 3, connect = {"Garden", "Main Hall"} },
    { name = "Foyer", x = 19, y = 10, w = 12, h = 2, connect = {"Main Hall"}  },
}

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

function onSetupBox(box)
    box.meeples:push(Meeple:new("player", "P0", "green"))
    box.meeples:push(Meeple:new("player", "P1", "yellow"))
    box.meeples:push(Meeple:new("player", "P2", "blue"))
    box.meeples:push(Meeple:new("player", "P3", "red"))

    box.meeples:push(Meeple:new("place", "red"))

    --printBox(box)

    return MAX_PLAYERS
end

function onSetupGame(players, box)
    local meeples = box.meeples:filter(function(meeple) return meeple.name == "player" end)
    assert(#meeples == MAX_PLAYERS)  -- not generally true, but is for this game

    for i = 1, #players do
        meeples[i].pos = "Foyer"
        players[i].meeples:push(meeples[i])
    end

    --print("player meeples")
    --printMeeples(pm)
end

function isMoveAllowed(player, meeple, start, dst)
    return true
end

function onMoveMeeple(player, meeple, start, dst)
end
