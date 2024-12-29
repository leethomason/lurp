local MAX_PLAYERS = 4

function onFetchBoard()
    return {
        { name = "Ballroom", x = 19, y = 0, w = 12, h = 3, connect = { "Garden", "Main Hall" } },
        { name = "Garden", x = 36, y = 0, w = 10, h = 3, connect = { "Ballroom", "Main Hall", "Lounge" } },
        { name = "Dining", x = 0, y = 5, w = 13, h = 3, connect = { "Main Hall" } },
        { name = "Main Hall", x = 19, y = 5, w = 12, h = 3, connect = { "Ballroom", "Garden", "Lounge", "Foyer", "Dining" } },
        { name = "Lounge", x = 36, y = 5, w = 10, h = 3, connect = {"Garden", "Main Hall"} },
        { name = "Foyer", x = 19, y = 10, w = 12, h = 2, connect = {"Main Hall"}  },
    }
end

function printMeeples(meeples)
    print("printMeeples")
    for _, v in ipairs(meeples) do
        print("meeples", v.type, v.id, v.color)
    end
end

function printBox(box)
    print("printBox")
    printMeeples(box.meeples)
end

function onSetupBox(box)
    box.meeples:push(Meeple:new("player", "P1", "green"))
    box.meeples:push(Meeple:new("player", "P2", "yellow"))
    box.meeples:push(Meeple:new("player", "P3", "blue"))
    box.meeples:push(Meeple:new("player", "P4", "red"))

    box.meeples:push(Meeple:new("place", "L", "red"))

    --printBox(box)

    return MAX_PLAYERS
end

function onSetupGame(players, box)
    local pm = box.meeples:filter(function(meeple) return meeple.type == "player" end)
    assert(#pm == MAX_PLAYERS)

    for i = 1, #players do
        pm[i].pos = "Foyer"
        --pm[i].color = "green"
        players[i].meeple = pm[i]
    end

    --print("player meeples")
    --printMeeples(pm)
end
