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

function printBox(box)
    print("printBox")
    for k, v in ipairs(box.meeples) do
        print("meeples", k, v)
    end
end

function onSetupBox(box)
    box.meeples:push(Meeple:new())

    printBox(box)
end
