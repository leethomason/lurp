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

-- Game creation: yes, 1st
-- Game load: no
function onSetupBox(box)
    local colors = { "red", "green", "blue", "yellow" }
    for i=1, MAX_PLAYERS do
        box.meeples:push(Meeple:new("player", "P" .. i, colors[i]))
    end

    box.meeples:push(Meeple:new("place", "red"))

    --printBox(box)

    return MAX_PLAYERS
end

-- Game creation: yes, 2nd
-- Game load: no
function onSetupGame(players, box)
    local meeples = box.meeples:filter(function(meeple) return meeple.name == "player" end)
    assert(#meeples == MAX_PLAYERS)  -- not generally true, but is for this game

    for i=1, #players do
        meeples[i].pos = "Foyer"
        players[i].meeples:push(meeples[i])
        --players[i].fear = Counter:new("fear", 0, 4, 1)
    end

    --print("player meeples")
    --printMeeples(pm)
end

-- Game creation: yes, 3rd
-- Game load: yes, 1st
function onInit(players, box)
    for i = 1, #players do
        players[i].fear.onMax = function() 
            print("Game Over for " .. players[i].name)
            --gameOver()
            --playerOver(players[i])
        end
    end
end

function isMoveAllowed(player, meeple, start, dst)
    print("Lua isMoveAllowed", player.index, meeple.name, start.name, dst.name)
    return true
end

function onMeepleMoved(player, meeple, start, dst)
    print("Lua onMoveMeeple", player.index, meeple.name, start.name, dst.name)
    --player.fear:inc()
end

